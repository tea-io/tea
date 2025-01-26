#include "tcp.h"
#include "../common/io.h"
#include "../common/log.h"
#include <fcntl.h>
#include <list>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <thread>

std::list<std::thread> threads;
std::list<int> clients;
std::list<SSL *> ssl_sessions;

static void client_handler(int fd, SSL *ssl, recv_handlers handlers) {
    while (true) {
        int err = handle_recv(fd, ssl, handlers);
        if (err < 0) {
            log(ERROR, fd, "Error handling message: %s", strerror(errno));
        }
        if (err == 0) {
            log(INFO, fd, "Closing connection");
            SSL_shutdown(ssl);
            close(fd);
            return;
        }
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-fd-leak"
int listen(int port, recv_handlers handlers, std::string cert, std::string key) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    SSL_CTX *ctx;
    SSL_load_error_strings();
    SSL_library_init();

    ctx = SSL_CTX_new(TLS_server_method());
    SSL_CTX_use_certificate_file(ctx, cert.c_str(), SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, key.c_str(), SSL_FILETYPE_PEM);
    SSL_CTX_load_verify_locations(ctx, cert.c_str(), nullptr);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    SSL_CTX_set_verify_depth(ctx, 4);
    SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);

    constexpr int one = 1;
    int err = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (err < 0) {
        log(ERROR, sock, "Error setting socket options: %s", strerror(errno));
        return 1;
    }
    if (bind(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        log(ERROR, sock, "Error bind port: %s", strerror(errno));
        return 1;
    }
    err = listen(sock, 10);
    if (err < 0) {
        log(ERROR, sock, "Error listening: %s", strerror(errno));
        return 1;
    }

    log(INFO, sock, "Listening on port %d", port);
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    while (true) {
        const int client_sock = accept(sock, reinterpret_cast<sockaddr *>(&client_addr), &client_addr_len);
        if (client_sock < 0) {
            log(ERROR, sock, "Error accepting connection: %s", strerror(errno));
            return 1;
        }

        clients.push_back(client_sock);
        SSL *ssl;
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client_sock);
        err = SSL_accept(ssl);
        if (err <= 0) {
            log(ERROR, sock, "SSL accept error");
            SSL_free(ssl);
            continue;
        }
        ssl_sessions.push_back(ssl);
        log(INFO, client_sock, "Accepted connection from %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        err = fcntl(client_sock, F_SETFL, fcntl(client_sock, F_GETFL) | O_NONBLOCK);
        if (err < 0) {
            log(ERROR, sock, "Error setting socket to non-blocking: %s", strerror(errno));
            return 1;
        }
        std::thread t(client_handler, client_sock, ssl, handlers);
        threads.push_back(std::move(t));
    }
    return 0;
}
#pragma GCC diagnostic pop

int close_connections() {
    for (auto &c : clients) {
        close(c);
    }
    for (auto &t : threads) {
        t.detach();
    }
    return 0;
}
