#include "tcp.h"
#include "../common/io.h"
#include "../common/log.h"
#include <condition_variable>
#include <google/protobuf/message.h>
#include <string>

std::map<int, std::string> messages;
std::map<int, std::condition_variable> conditions;
std::map<int, std::mutex> mutexes;
int requst_id = 0;

template <typename T> int response_handler(int sock, int id, T message) {
    (void)sock;
    std::unique_lock<std::mutex> lock(mutexes[id]);
    messages[id] = message->SerializeAsString();
    conditions[id].notify_all();
    lock.unlock();
    return 0;
}

template <typename T> int request_handler(int sock, int id, T message) {
    (void)sock;
    (void)id;
    (void)message;
    return 0;
}

recv_handlers handlers = {
    .init_request = request_handler<InitRequest *>,
    .init_response = response_handler<InitResponse *>,
    .get_attr_request = request_handler<GetAttrRequest *>,
    .get_attr_response = response_handler<GetAttrResponse *>,
    .open_request = request_handler<OpenRequest *>,
    .open_response = response_handler<OpenResponse *>,
    .release_request = request_handler<ReleaseRequest *>,
    .release_response = response_handler<ReleaseResponse *>,
    .read_dir_request = request_handler<ReadDirRequest *>,
    .read_dir_response = response_handler<ReadDirResponse *>,
    .read_request = request_handler<ReadRequest *>,
    .read_response = response_handler<ReadResponse *>,
    .write_request = request_handler<WriteRequest *>,
    .write_response = response_handler<WriteResponse *>,
    .create_request = request_handler<CreateRequest *>,
    .create_response = response_handler<CreateResponse *>,
    .mkdir_request = request_handler<MkdirRequest *>,
    .mkdir_response = response_handler<MkdirResponse *>,
    .unlink_request = request_handler<UnlinkRequest *>,
    .unlink_response = response_handler<UnlinkResponse *>,
    .rmdir_request = request_handler<RmdirRequest *>,
    .rmdir_response = response_handler<RmdirResponse *>,
    .rename_request = request_handler<RenameRequest *>,
    .rename_response = response_handler<RenameResponse *>,
};

int connect(std::string host, int port) {
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log(ERROR, sock, "Error creating socket: %s", strerror(errno));
        return 1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host.c_str());
    if (connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        log(ERROR, sock, "Error connecting to port %d: %s", port, strerror(errno));
        return 1;
    }
    log(INFO, sock, "Connected to port %d", port);
    return sock;
}

int recv_thread(int sock) {
    while (true) {
        int err = handle_recv(sock, handlers);
        if (err < 0) {
            log(ERROR, sock, "Error handling message");
            return -1;
        }
        if (err == 0) {
            close(sock);
            log(INFO, sock, "Closing connection");
            exit(1);
            return 0;
        }
    }
    return 1;
}
