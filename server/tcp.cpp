#include "../common/io.h"
#include "../common/log.h"
#include <list>
#include <thread>

std::list<std::thread> threads;
std::list<int> clients;

void client_handler(int fd, recv_handlers handlers) {
    while (true) {
        int err = handle_recv(fd, handlers);
        if (err < 0) {
            log(ERROR, fd, "Error handling message");
            return;
        }
        if (err == 0) {
            log(INFO, fd, "Closing connection");
            close(fd);
            return;
        }
    }
}

int listen(int port, recv_handlers handlers) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    constexpr int one = 1;
    int err = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (err < 0) {
        log(ERROR, sock, "Error setting socket options: %s", strerror(errno));
        return 1;
    }
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
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
        const int client_sock = accept(sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            log(ERROR, sock, "Error accepting connection: %s", strerror(errno));
            return 1;
        }
        clients.push_back(client_sock);
        log(INFO, client_sock, "Accepted connection from %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        std::thread t(client_handler, client_sock, handlers);
        threads.push_back(std::move(t));
    }
    return 0;
}

int close_connections() {
    for (auto &c : clients) {
        close(c);
    }
    for (auto &t : threads) {
        t.detach();
    }
    return 0;
}
