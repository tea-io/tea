#include "tcp.h"
#include "../common/io.h"
#include "../common/log.h"
#include <condition_variable>
#include <google/protobuf/message.h>
#include <string>

std::map<int, std::string> messages;
std::map<int, std::condition_variable> conditions;
std::mutex condition_mutex;
int requst_id = 0;

template <typename T> int response_handler(int sock, int id, T message) {
    (void)sock;
    std::unique_lock<std::mutex> lock(condition_mutex);
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
    .chmod_request = request_handler<ChmodRequest *>,
    .chmod_response = response_handler<ChmodResponse *>,
    .truncate_request = request_handler<TruncateRequest *>,
    .truncate_response = response_handler<TruncateResponse *>,
    .mknod_request = request_handler<MknodRequest *>,
    .mknod_response = response_handler<MknodResponse *>,
    .link_request = request_handler<LinkRequest *>,
    .link_response = response_handler<LinkResponse *>,
    .symlink_request = request_handler<SymlinkRequest *>,
    .symlink_response = response_handler<SymlinkResponse *>,
    .read_link_request = request_handler<ReadLinkRequest *>,
    .read_link_response = response_handler<ReadLinkResponse *>,
    .statfs_request = request_handler<StatfsRequest *>,
    .statfs_response = response_handler<StatfsResponse *>,
    .fsync_request = request_handler<FsyncRequest *>,
    .fsync_response = response_handler<FsyncResponse *>,
    .setxattr_request = request_handler<SetxattrRequest *>,
    .setxattr_response = response_handler<SetxattrResponse *>,
    .getxattr_request = request_handler<GetxattrRequest *>,
    .getxattr_response = response_handler<GetxattrResponse *>,
    .listxattr_request = request_handler<ListxattrRequest *>,
    .listxattr_response = response_handler<ListxattrResponse *>,
    .removexattr_request = request_handler<RemovexattrRequest *>,
    .removexattr_response = response_handler<RemovexattrResponse *>,
    .opendir_request = request_handler<OpendirRequest *>,
    .opendir_response = response_handler<OpendirResponse *>,
    .releasedir_request = request_handler<ReleasedirRequest *>,
    .releasedir_response = response_handler<ReleasedirResponse *>,
    .fsyncdir_request = request_handler<FsyncdirRequest *>,
    .fsyncdir_response = response_handler<FsyncdirResponse *>,
    .utimens_request = request_handler<UtimensRequest *>,
    .utimens_response = response_handler<UtimensResponse *>,
    .access_request = request_handler<AccessRequest *>,
    .access_response = response_handler<AccessResponse *>,
    .lock_request = request_handler<LockRequest *>,
    .lock_response = response_handler<LockResponse *>,
    .flock_request = request_handler<FlockRequest *>,
    .flock_response = response_handler<FlockResponse *>,
    .fallocate_request = request_handler<FallocateRequest *>,
    .fallocate_response = response_handler<FallocateResponse *>,
    .lseek_request = request_handler<LseekRequest *>,
    .lseek_response = response_handler<LseekResponse *>,
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
        return -1;
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
