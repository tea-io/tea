#include "tcp.h"
#include "../common/io.h"
#include "../common/log.h"
#include <list>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>

std::list<std::thread> threads;
std::list<int> clients;

static void client_handler(int fd, recv_handlers handlers) {
    while (true) {
        int err = handle_recv(fd, handlers);
        if (err < 0) {
            log(ERROR, fd, "Error handling message");
        }
        if (err == 0) {
            log(INFO, fd, "Closing connection");
            close(fd);
            return;
        }
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-fd-leak"
static int accept_connections(int sock, recv_handlers handlers) {
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    while (true) {
        const int client_sock = accept(sock, reinterpret_cast<sockaddr *>(&client_addr), &client_addr_len);
        if (client_sock < 0) {
            log(ERROR, sock, "Error accepting connection: %s", strerror(errno));
            return 1;
        }
        clients.push_back(client_sock);
        log(INFO, client_sock, "Accepted connection from %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        std::thread t(client_handler, client_sock, handlers);
        threads.push_back(std::move(t));
    }
}
#pragma GCC diagnostic pop

int listen(int port, recv_handlers fs, recv_handlers event) {
    int sock = listen(port);
    if (sock < 0) {
        return -1;
    }
    int notify_sock = listen(port + 1);
    if (notify_sock < 0) {
        return -2;
    }
    log(INFO, sock, "Listening on ports %d %d", port, port + 1);
    std::thread t([notify_sock, event] { accept_connections(notify_sock, event); });
    threads.push_back(std::move(t));
    accept_connections(sock, fs);
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
