#include "tcp.h"
#include "../common/log.h"
#include "../common/tcp.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>

std::thread event_thread;

int listen(int port, recv_handlers fs, recv_handlers event) {
    int sock = listen(port);
    if (sock < 0) {
        return -1;
    }
    int notify_sock = listen(port - 1);
    if (notify_sock < 0) {
        return -2;
    }
    log(INFO, sock, "Listening on ports %d %d", port, port - 1);
    event_thread = std::thread(accept_connections, notify_sock, event, false, EVENT);
    accept_connections(sock, fs, false, FS);
    return 0;
}

int close() {
    event_thread.detach();
    close_connections();
    return 0;
}
