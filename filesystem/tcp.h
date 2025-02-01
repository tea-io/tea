#pragma once
#include "../common/io.h"
#include "../common/log.h"
#include <condition_variable>
#include <gnutls/gnutls.h>
#include <mutex>
#include <string>

extern int request_id;
extern std::map<int, std::string> messages;
extern std::map<int, std::condition_variable> conditions;
extern std::mutex condition_mutex;

int connect(std::string host, int port);

int recv_thread(gnutls_session_t ssl, int sock);

int listen_lsp(int port, int server_sock, gnutls_session_t ssl);

template <typename T> int request_response(int sock, gnutls_session_t ssl, google::protobuf::Message &request, T *response, Type type) {
    int id = ++request_id;
    int err = send_message(sock, ssl, id, type, &request);
    if (err < 0) {
        return -1;
    }
    std::unique_lock<std::mutex> lock(condition_mutex);
    if (conditions.find(id) == conditions.end()) {
        conditions[id].wait(lock);
    }
    response->ParseFromString(messages[id]);
    return 0;
}
