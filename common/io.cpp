#include "io.h"
#include "../proto/messages.pb.h"
#include "header.h"
#include "log.h"
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <google/protobuf/message.h>

int send_message(int sock, int id, Type type, google::protobuf::Message *body) {
    Header header;
    header.size = body->ByteSizeLong();
    header.id = id;
    header.type = type;

    char *header_buffer = serialize(&header);
    char *message_buffer = new char[HEADER_SIZE + body->ByteSizeLong()];
    memcpy(message_buffer, header_buffer, HEADER_SIZE);
    delete[] header_buffer;

    char *body_buffer = new char[body->ByteSizeLong()];
    bool err = body->SerializeToArray(body_buffer, body->ByteSizeLong());
    if (!err) {
        log(DEBUG, sock, "(%d) Serialize body failed", id);
        return -1;
    }
    memcpy(message_buffer + HEADER_SIZE, body_buffer, body->ByteSizeLong());
    delete[] body_buffer;

    int len = send(sock, message_buffer, HEADER_SIZE + body->ByteSizeLong(), 0);
    delete[] message_buffer;
    if (len < 0) {
        log(DEBUG, sock, "(%d) Send message failed: %s", id, strerror(errno));
        return -1;
    }
    std::string debug = body->DebugString();
    log(DEBUG, sock, "(%d) Send message success: %s - %d bytes", id, debug.c_str(), len);
    return len;
}

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

template <typename T> int recv_handler_caller(char *recv_buffer, Header *header, int sock, int (*handler)(int sock, int id, T *response)) {
    T request;
    request.ParseFromArray(recv_buffer, header->size);
    std::string type = typeid(T).name();
    log(DEBUG, sock, "(%d) Recieved: %s\n %s", header->id, type.c_str(), request.DebugString().c_str());
    int ret = handler(sock, header->id, &request);
    return ret;
}

// Handle recv messages, 1 on success, 0 on EOF, -1 on error
int handle_recv(int sock, recv_handlers &handlers) {
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
    int ret = -2;
    switch (header->type) {
    case Type::INIT_REQUEST: {
        ret = recv_handler_caller<InitRequest>(recv_buffer, header, sock, handlers.init_request);
        break;
    }
    case Type::INIT_RESPONSE: {
        ret = recv_handler_caller<InitResponse>(recv_buffer, header, sock, handlers.init_response);
        break;
    }
    case Type::GET_ATTR_REQUEST: {
        ret = recv_handler_caller<GetAttrRequest>(recv_buffer, header, sock, handlers.get_attr_request);
        break;
    }
    case Type::GET_ATTR_RESPONSE: {
        ret = recv_handler_caller<GetAttrResponse>(recv_buffer, header, sock, handlers.get_attr_response);
        break;
    }
    case Type::OPEN_REQUEST: {
        ret = recv_handler_caller<OpenRequest>(recv_buffer, header, sock, handlers.open_request);
        break;
    }
    case Type::OPEN_RESPONSE: {
        ret = recv_handler_caller<OpenResponse>(recv_buffer, header, sock, handlers.open_response);
        break;
    }
    case Type::RELEASE_REQUEST: {
        ret = recv_handler_caller<ReleaseRequest>(recv_buffer, header, sock, handlers.release_request);
        break;
    }
    case Type::RELEASE_RESPONSE: {
        ret = recv_handler_caller<ReleaseResponse>(recv_buffer, header, sock, handlers.release_response);
        break;
    }
    case Type::READ_DIR_REQUEST: {
        ret = recv_handler_caller<ReadDirRequest>(recv_buffer, header, sock, handlers.read_dir_request);
        break;
    }
    case Type::READ_DIR_RESPONSE: {
        ret = recv_handler_caller<ReadDirResponse>(recv_buffer, header, sock, handlers.read_dir_response);
        break;
    }
    case Type::READ_REQUEST: {
        ret = recv_handler_caller<ReadRequest>(recv_buffer, header, sock, handlers.read_request);
        break;
    }
    case Type::READ_RESPONSE: {
        ret = recv_handler_caller<ReadResponse>(recv_buffer, header, sock, handlers.read_response);
        break;
    }
    case Type::WRITE_REQUEST: {
        ret = recv_handler_caller<WriteRequest>(recv_buffer, header, sock, handlers.write_request);
        break;
    }
    case Type::WRITE_RESPONSE: {
        ret = recv_handler_caller<WriteResponse>(recv_buffer, header, sock, handlers.write_response);
        break;
    }
    case Type::CREATE_REQUEST: {
        ret = recv_handler_caller<CreateRequest>(recv_buffer, header, sock, handlers.create_request);
        break;
    }
    case Type::CREATE_RESPONSE: {
        ret = recv_handler_caller<CreateResponse>(recv_buffer, header, sock, handlers.create_response);
        break;
    }
    case Type::MKDIR_REQUEST: {
        ret = recv_handler_caller<MkdirRequest>(recv_buffer, header, sock, handlers.mkdir_request);
        break;
    }
    case Type::MKDIR_RESPONSE: {
        ret = recv_handler_caller<MkdirResponse>(recv_buffer, header, sock, handlers.mkdir_response);
        break;
    }
    case Type::UNLINK_REQUEST: {
        ret = recv_handler_caller<UnlinkRequest>(recv_buffer, header, sock, handlers.unlink_request);
        break;
    }
    case Type::UNLINK_RESPONSE: {
        ret = recv_handler_caller<UnlinkResponse>(recv_buffer, header, sock, handlers.unlink_response);
        break;
    }
    case Type::RMDIR_REQUEST: {
        ret = recv_handler_caller<RmdirRequest>(recv_buffer, header, sock, handlers.rmdir_request);
        break;
    }
    case Type::RMDIR_RESPONSE: {
        ret = recv_handler_caller<RmdirResponse>(recv_buffer, header, sock, handlers.rmdir_response);
        break;
    }
    case Type::RENAME_REQUEST: {
        ret = recv_handler_caller<RenameRequest>(recv_buffer, header, sock, handlers.rename_request);
        break;
    }
    case Type::RENAME_RESPONSE: {
        ret = recv_handler_caller<RenameResponse>(recv_buffer, header, sock, handlers.rename_response);
        break;
    }
    case Type::CHMOD_REQUEST: {
        ret = recv_handler_caller<ChmodRequest>(recv_buffer, header, sock, handlers.chmod_request);
        break;
    }
    case Type::CHMOD_RESPONSE: {
        ret = recv_handler_caller<ChmodResponse>(recv_buffer, header, sock, handlers.chmod_response);
        break;
    }
    case Type::TRUNCATE_REQUEST: {
        ret = recv_handler_caller<TruncateRequest>(recv_buffer, header, sock, handlers.truncate_request);
        break;
    }
    case Type::TRUNCATE_RESPONSE: {
        ret = recv_handler_caller<TruncateResponse>(recv_buffer, header, sock, handlers.truncate_response);
        break;
    }
    case Type::MKNOD_REQUEST: {
        ret = recv_handler_caller<MknodRequest>(recv_buffer, header, sock, handlers.mknod_request);
        break;
    }
    case Type::MKNOD_RESPONSE: {
        ret = recv_handler_caller<MknodResponse>(recv_buffer, header, sock, handlers.mknod_response);
        break;
    }
    case Type::LINK_REQUEST: {
        ret = recv_handler_caller<LinkRequest>(recv_buffer, header, sock, handlers.link_request);
        break;
    }
    case Type::LINK_RESPONSE: {
        ret = recv_handler_caller<LinkResponse>(recv_buffer, header, sock, handlers.link_response);
        break;
    }
    case Type::SYMLINK_REQUEST: {
        ret = recv_handler_caller<SymlinkRequest>(recv_buffer, header, sock, handlers.symlink_request);
        break;
    }
    case Type::SYMLINK_RESPONSE: {
        ret = recv_handler_caller<SymlinkResponse>(recv_buffer, header, sock, handlers.symlink_response);
        break;
    }
    case Type::READ_LINK_REQUEST: {
        ret = recv_handler_caller<ReadLinkRequest>(recv_buffer, header, sock, handlers.read_link_request);
        break;
    }
    case Type::READ_LINK_RESPONSE: {
        ret = recv_handler_caller<ReadLinkResponse>(recv_buffer, header, sock, handlers.read_link_response);
        break;
    }
    case Type::STATFS_REQUEST: {
        ret = recv_handler_caller<StatfsRequest>(recv_buffer, header, sock, handlers.statfs_request);
        break;
    }
    case Type::STATFS_RESPONSE: {
        ret = recv_handler_caller<StatfsResponse>(recv_buffer, header, sock, handlers.statfs_response);
        break;
    }
    case Type::FSYNC_REQUEST: {
        ret = recv_handler_caller<FsyncRequest>(recv_buffer, header, sock, handlers.fsync_request);
        break;
    }
    case Type::FSYNC_RESPONSE: {
        ret = recv_handler_caller<FsyncResponse>(recv_buffer, header, sock, handlers.fsync_response);
        break;
    }
    default: {
        log(DEBUG, sock, "(%d) Unknown message type: %d", header->id, header->type);
        break;
    }
    }
    if (ret < 0) {
        log(DEBUG, sock, "Handler failed: %d", ret);
    } else {
        log(DEBUG, sock, "Handler success: %d", ret);
        ret = 1;
    }
    delete header;
    delete[] recv_buffer;
    return ret;
};
