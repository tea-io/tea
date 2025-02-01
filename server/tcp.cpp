#include "tcp.h"
#include "../common/io.h"
#include "../common/log.h"
#include <fcntl.h>
#include <gnutls/compat.h>
#include <gnutls/gnutls.h>
#include <list>
#include <sys/socket.h>
#include <thread>

std::list<std::thread> threads;
std::list<int> clients;
std::list<gnutls_session_t> ssl_sessions;

static void client_handler(int fd, gnutls_session_t ssl, recv_handlers handlers) {
    while (true) {
        int err = handle_recv(fd, ssl, handlers);
        if (err < 0) {
            log(ERROR, fd, "Error handling message: %s", strerror(errno));
        }
        if (err == 0) {
            log(INFO, fd, "Closing connection");
            gnutls_bye(ssl, GNUTLS_SHUT_RDWR);
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

    gnutls_certificate_credentials_t xcred;
    gnutls_global_init();
    gnutls_certificate_allocate_credentials(&xcred);
    gnutls_certificate_set_x509_key_file(xcred, cert.c_str(), key.c_str(), GNUTLS_X509_FMT_PEM);
    gnutls_certificate_set_x509_system_trust(xcred);
    gnutls_certificate_set_x509_trust_file(xcred, cert.c_str(), GNUTLS_X509_FMT_PEM);

    gnutls_priority_t priority_cache;
    gnutls_priority_init(&priority_cache, "NORMAL", NULL);

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

        gnutls_session_t ssl_session;
        gnutls_init(&ssl_session, GNUTLS_SERVER);
        gnutls_set_default_priority(ssl_session);
        gnutls_session_set_verify_cert(ssl_session, NULL, 0);
        gnutls_credentials_set(ssl_session, GNUTLS_CRD_CERTIFICATE, xcred);

        gnutls_certificate_server_set_request(ssl_session, GNUTLS_CERT_REQUEST);

        const int client_sock = accept(sock, reinterpret_cast<sockaddr *>(&client_addr), &client_addr_len);
        if (client_sock < 0) {
            log(ERROR, sock, "Error accepting connection: %s", strerror(errno));
            return 1;
        }

        gnutls_transport_set_int(ssl_session, client_sock);

        clients.push_back(client_sock);

        err = gnutls_handshake(ssl_session);
        if (err < 0) {
            log(ERROR, sock, "GnuTLS handshake failed: %s", gnutls_strerror(err));
            gnutls_bye(ssl_session, GNUTLS_SHUT_RDWR);
            gnutls_deinit(ssl_session);
            continue;
        }
        ssl_sessions.push_back(ssl_session);
        log(INFO, client_sock, "Accepted connection from %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        std::thread t(client_handler, client_sock, ssl_session, handlers);
        threads.push_back(std::move(t));
    }
    gnutls_certificate_free_credentials(xcred);
    gnutls_global_deinit();
    return 0;
}
#pragma GCC diagnostic pop

int close_connections() {
    for (auto &c : clients) {
        close(c);
    }
    for (auto &s : ssl_sessions) {
        gnutls_bye(s, GNUTLS_SHUT_RDWR);
        gnutls_deinit(s);
    }
    for (auto &t : threads) {
        t.detach();
    }
    return 0;
}
