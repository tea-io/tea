#include "tcp.h"
#include "../proto/messages.pb.h"
#include "handlers.h"
#include "header.h"
#include "log.h"
#include <absl/strings/string_view.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <thread>

std::list<std::thread> threads;
std::list<int> clients;

int listen(int port) {
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
    if (bind(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        log(ERROR, sock, "Error bind port: %s", strerror(errno));
        return 1;
    }
    err = listen(sock, 10);
    if (err < 0) {
        log(ERROR, sock, "Error listening: %s", strerror(errno));
        return 1;
    }
    return sock;
}

int send_message(int sock, int id, Type type, google::protobuf::Message *body, bool json) {
    Header header;
    header.id = id;
    header.type = type;

    char *body_buffer;
    if (json) {
        std::string request;
        absl::Status status = google::protobuf::util::MessageToJsonString(*body, &request);
        if (!status.ok()) {
            log(ERROR, sock, "(%d) Serialize body failed: %s", id, std::string(status.message()).c_str());
            return -1;
        }
        header.size = request.size();
        body_buffer = new char[request.size()];
        memcpy(body_buffer, request.c_str(), request.size());
    } else {
        body_buffer = new char[body->ByteSizeLong()];
        bool err = body->SerializeToArray(body_buffer, body->ByteSizeLong());
        if (!err) {
            log(DEBUG, sock, "(%d) Serialize body failed", id);
            return -1;
        }
        header.size = body->ByteSizeLong();
    }

    char *message_buffer;
    message_buffer = new char[HEADER_SIZE + header.size];
    char *header_buffer = serialize(&header);
    memcpy(message_buffer, header_buffer, HEADER_SIZE);
    delete[] header_buffer;
    memcpy(message_buffer + HEADER_SIZE, body_buffer, header.size);
    delete[] body_buffer;
    int len = send(sock, message_buffer, HEADER_SIZE + header.size, 0);
    delete[] message_buffer;
    if (len < 0) {
        log(DEBUG, sock, "(%d) Send message failed: %s", id, strerror(errno));
        return -1;
    }
    std::string debug = body->DebugString();
    log(DEBUG, sock, "(%d) Send message success: %s - %d bytes", id, debug.c_str(), len);
    return len;
}

int send_message(int sock, int id, Type type, google::protobuf::Message *body) { return send_message(sock, id, type, body, false); }

int full_read(int fd, char &buf, int size) {
    int recived = 0;
    while (recived < size) {
        int len = recv(fd, &buf + recived, size - recived, 0);
        if (len == 0) {
            log(DEBUG, fd, "EOF");
            return 0;
        }
        if (len < 0) {
            log(DEBUG, fd, "Full read failed: %s", strerror(errno));
            return -1;
        }
        recived += len;
    }
    return recived;
}

int handle_recv(int sock, recv_handlers &handlers, bool json, socket_type type) {
    char buffer[HEADER_SIZE];
    int recived = full_read(sock, *buffer, sizeof(buffer));
    if (recived == 0) {
        return 0;
    }
    if (recived < static_cast<int>(sizeof(buffer))) {
        log(DEBUG, sock, "Full header read failed");
        return -1;
    }
    Header *header = new Header();
    deserialize(buffer, header);
    log(DEBUG, sock, "Received header: size %d id %d type %d %d bytes", header->size, header->id, header->type, recived);
    char *recv_buffer = new char[header->size];
    recived = full_read(sock, *recv_buffer, header->size);
    if (recived == 0 && header->size != 0) {
        return 0;
    }
    if (recived < header->size) {
        return -1;
    }
    int ret = -3;
    switch (type) {
    case FS: {
        ret = fs_handle_recv(sock, header, recv_buffer, handlers.fs, json);
        break;
    }
    case EVENT: {
        ret = event_handle_recv(sock, header, recv_buffer, handlers.event, json);
        break;
    }
    }
    delete header;
    delete[] recv_buffer;
    return ret;
};

void recv_therad(int fd, recv_handlers handlers, bool json, socket_type type) {
    while (true) {
        int err = handle_recv(fd, handlers, json, type);
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
int accept_connections(int sock, recv_handlers handlers, bool json, socket_type type) {
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
        std::thread t(recv_therad, client_sock, handlers, json, type);
        threads.push_back(std::move(t));
    }
}
#pragma GCC diagnostic pop

int close_connections() {
    for (int client : clients) {
        close(client);
    }
    for (std::thread &t : threads) {
        t.join();
    }
    return 0;
}
