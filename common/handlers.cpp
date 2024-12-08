#include "handlers.h"
#include "../proto/messages.pb.h"
#include "header.h"
#include "log.h"
#include <google/protobuf/util/json_util.h>

template <typename T> int recv_handler_caller(char *recv_buffer, Header *header, int sock, int (*handler)(int sock, int id, T *response), bool json) {
    T request;
    if (json) {
        absl::string_view str(recv_buffer, header->size);
        absl::Status status = google::protobuf::util::JsonStringToMessage(str, &request);
        if (!status.ok()) {
            log(ERROR, sock, "JsonStringToMessage failed: %s", std::string(status.message()).c_str());
            return -1;
        }
    } else {
        request.ParseFromArray(recv_buffer, header->size);
    }
    std::string type = typeid(T).name();
    log(DEBUG, sock, "(%d) Recieved: %s\n %s", header->id, type.c_str(), request.DebugString().c_str());
    int ret = handler(sock, header->id, &request);
    return ret;
}

int fs_handle_recv(int sock, Header *header, char *recv_buffer, fs_handlers &handlers, bool json) {
    int ret = -2;
    switch (header->type) {
    case Type::INIT_REQUEST: {
        ret = recv_handler_caller<InitRequest>(recv_buffer, header, sock, handlers.init_request, json);
        break;
    }
    case Type::INIT_RESPONSE: {
        ret = recv_handler_caller<InitResponse>(recv_buffer, header, sock, handlers.init_response, json);
        break;
    }
    case Type::GET_ATTR_REQUEST: {
        ret = recv_handler_caller<GetAttrRequest>(recv_buffer, header, sock, handlers.get_attr_request, json);
        break;
    }
    case Type::GET_ATTR_RESPONSE: {
        ret = recv_handler_caller<GetAttrResponse>(recv_buffer, header, sock, handlers.get_attr_response, json);
        break;
    }
    case Type::OPEN_REQUEST: {
        ret = recv_handler_caller<OpenRequest>(recv_buffer, header, sock, handlers.open_request, json);
        break;
    }
    case Type::OPEN_RESPONSE: {
        ret = recv_handler_caller<OpenResponse>(recv_buffer, header, sock, handlers.open_response, json);
        break;
    }
    case Type::RELEASE_REQUEST: {
        ret = recv_handler_caller<ReleaseRequest>(recv_buffer, header, sock, handlers.release_request, json);
        break;
    }
    case Type::RELEASE_RESPONSE: {
        ret = recv_handler_caller<ReleaseResponse>(recv_buffer, header, sock, handlers.release_response, json);
        break;
    }
    case Type::READ_DIR_REQUEST: {
        ret = recv_handler_caller<ReadDirRequest>(recv_buffer, header, sock, handlers.read_dir_request, json);
        break;
    }
    case Type::READ_DIR_RESPONSE: {
        ret = recv_handler_caller<ReadDirResponse>(recv_buffer, header, sock, handlers.read_dir_response, json);
        break;
    }
    case Type::READ_REQUEST: {
        ret = recv_handler_caller<ReadRequest>(recv_buffer, header, sock, handlers.read_request, json);
        break;
    }
    case Type::READ_RESPONSE: {
        ret = recv_handler_caller<ReadResponse>(recv_buffer, header, sock, handlers.read_response, json);
        break;
    }
    case Type::WRITE_REQUEST: {
        ret = recv_handler_caller<WriteRequest>(recv_buffer, header, sock, handlers.write_request, json);
        break;
    }
    case Type::WRITE_RESPONSE: {
        ret = recv_handler_caller<WriteResponse>(recv_buffer, header, sock, handlers.write_response, json);
        break;
    }
    case Type::CREATE_REQUEST: {
        ret = recv_handler_caller<CreateRequest>(recv_buffer, header, sock, handlers.create_request, json);
        break;
    }
    case Type::CREATE_RESPONSE: {
        ret = recv_handler_caller<CreateResponse>(recv_buffer, header, sock, handlers.create_response, json);
        break;
    }
    case Type::MKDIR_REQUEST: {
        ret = recv_handler_caller<MkdirRequest>(recv_buffer, header, sock, handlers.mkdir_request, json);
        break;
    }
    case Type::MKDIR_RESPONSE: {
        ret = recv_handler_caller<MkdirResponse>(recv_buffer, header, sock, handlers.mkdir_response, json);
        break;
    }
    case Type::UNLINK_REQUEST: {
        ret = recv_handler_caller<UnlinkRequest>(recv_buffer, header, sock, handlers.unlink_request, json);
        break;
    }
    case Type::UNLINK_RESPONSE: {
        ret = recv_handler_caller<UnlinkResponse>(recv_buffer, header, sock, handlers.unlink_response, json);
        break;
    }
    case Type::RMDIR_REQUEST: {
        ret = recv_handler_caller<RmdirRequest>(recv_buffer, header, sock, handlers.rmdir_request, json);
        break;
    }
    case Type::RMDIR_RESPONSE: {
        ret = recv_handler_caller<RmdirResponse>(recv_buffer, header, sock, handlers.rmdir_response, json);
        break;
    }
    case Type::RENAME_REQUEST: {
        ret = recv_handler_caller<RenameRequest>(recv_buffer, header, sock, handlers.rename_request, json);
        break;
    }
    case Type::RENAME_RESPONSE: {
        ret = recv_handler_caller<RenameResponse>(recv_buffer, header, sock, handlers.rename_response, json);
        break;
    }
    case Type::CHMOD_REQUEST: {
        ret = recv_handler_caller<ChmodRequest>(recv_buffer, header, sock, handlers.chmod_request, json);
        break;
    }
    case Type::CHMOD_RESPONSE: {
        ret = recv_handler_caller<ChmodResponse>(recv_buffer, header, sock, handlers.chmod_response, json);
        break;
    }
    case Type::TRUNCATE_REQUEST: {
        ret = recv_handler_caller<TruncateRequest>(recv_buffer, header, sock, handlers.truncate_request, json);
        break;
    }
    case Type::TRUNCATE_RESPONSE: {
        ret = recv_handler_caller<TruncateResponse>(recv_buffer, header, sock, handlers.truncate_response, json);
        break;
    }
    case Type::MKNOD_REQUEST: {
        ret = recv_handler_caller<MknodRequest>(recv_buffer, header, sock, handlers.mknod_request, json);
        break;
    }
    case Type::MKNOD_RESPONSE: {
        ret = recv_handler_caller<MknodResponse>(recv_buffer, header, sock, handlers.mknod_response, json);
        break;
    }
    case Type::LINK_REQUEST: {
        ret = recv_handler_caller<LinkRequest>(recv_buffer, header, sock, handlers.link_request, json);
        break;
    }
    case Type::LINK_RESPONSE: {
        ret = recv_handler_caller<LinkResponse>(recv_buffer, header, sock, handlers.link_response, json);
        break;
    }
    case Type::SYMLINK_REQUEST: {
        ret = recv_handler_caller<SymlinkRequest>(recv_buffer, header, sock, handlers.symlink_request, json);
        break;
    }
    case Type::SYMLINK_RESPONSE: {
        ret = recv_handler_caller<SymlinkResponse>(recv_buffer, header, sock, handlers.symlink_response, json);
        break;
    }
    case Type::READ_LINK_REQUEST: {
        ret = recv_handler_caller<ReadLinkRequest>(recv_buffer, header, sock, handlers.read_link_request, json);
        break;
    }
    case Type::READ_LINK_RESPONSE: {
        ret = recv_handler_caller<ReadLinkResponse>(recv_buffer, header, sock, handlers.read_link_response, json);
        break;
    }
    case Type::STATFS_REQUEST: {
        ret = recv_handler_caller<StatfsRequest>(recv_buffer, header, sock, handlers.statfs_request, json);
        break;
    }
    case Type::STATFS_RESPONSE: {
        ret = recv_handler_caller<StatfsResponse>(recv_buffer, header, sock, handlers.statfs_response, json);
        break;
    }
    case Type::FSYNC_REQUEST: {
        ret = recv_handler_caller<FsyncRequest>(recv_buffer, header, sock, handlers.fsync_request, json);
        break;
    }
    case Type::FSYNC_RESPONSE: {
        ret = recv_handler_caller<FsyncResponse>(recv_buffer, header, sock, handlers.fsync_response, json);
        break;
    }
    case Type::SETXATTR_REQUEST: {
        ret = recv_handler_caller<SetxattrRequest>(recv_buffer, header, sock, handlers.setxattr_request, json);
        break;
    }
    case Type::SETXATTR_RESPONSE: {
        ret = recv_handler_caller<SetxattrResponse>(recv_buffer, header, sock, handlers.setxattr_response, json);
        break;
    }
    case Type::GETXATTR_REQUEST: {
        ret = recv_handler_caller<GetxattrRequest>(recv_buffer, header, sock, handlers.getxattr_request, json);
        break;
    }
    case Type::GETXATTR_RESPONSE: {
        ret = recv_handler_caller<GetxattrResponse>(recv_buffer, header, sock, handlers.getxattr_response, json);
        break;
    }
    case Type::LISTXATTR_REQUEST: {
        ret = recv_handler_caller<ListxattrRequest>(recv_buffer, header, sock, handlers.listxattr_request, json);
        break;
    }
    case Type::LISTXATTR_RESPONSE: {
        ret = recv_handler_caller<ListxattrResponse>(recv_buffer, header, sock, handlers.listxattr_response, json);
        break;
    }
    case Type::REMOVEXATTR_REQUEST: {
        ret = recv_handler_caller<RemovexattrRequest>(recv_buffer, header, sock, handlers.removexattr_request, json);
        break;
    }
    case Type::REMOVEXATTR_RESPONSE: {
        ret = recv_handler_caller<RemovexattrResponse>(recv_buffer, header, sock, handlers.removexattr_response, json);
        break;
    }
    case Type::OPENDIR_REQUEST: {
        ret = recv_handler_caller<OpendirRequest>(recv_buffer, header, sock, handlers.opendir_request, json);
        break;
    }
    case Type::OPENDIR_RESPONSE: {
        ret = recv_handler_caller<OpendirResponse>(recv_buffer, header, sock, handlers.opendir_response, json);
        break;
    }
    case Type::RELEASEDIR_REQUEST: {
        ret = recv_handler_caller<ReleasedirRequest>(recv_buffer, header, sock, handlers.releasedir_request, json);
        break;
    }
    case Type::RELEASEDIR_RESPONSE: {
        ret = recv_handler_caller<ReleasedirResponse>(recv_buffer, header, sock, handlers.releasedir_response, json);
        break;
    }
    case Type::FSYNCDIR_REQUEST: {
        ret = recv_handler_caller<FsyncdirRequest>(recv_buffer, header, sock, handlers.fsyncdir_request, json);
        break;
    }
    case Type::FSYNCDIR_RESPONSE: {
        ret = recv_handler_caller<FsyncdirResponse>(recv_buffer, header, sock, handlers.fsyncdir_response, json);
        break;
    }
    case Type::UTIMENS_REQUEST: {
        ret = recv_handler_caller<UtimensRequest>(recv_buffer, header, sock, handlers.utimens_request, json);
        break;
    }
    case Type::UTIMENS_RESPONSE: {
        ret = recv_handler_caller<UtimensResponse>(recv_buffer, header, sock, handlers.utimens_response, json);
        break;
    }
    case Type::ACCESS_REQUEST: {
        ret = recv_handler_caller<AccessRequest>(recv_buffer, header, sock, handlers.access_request, json);
        break;
    }
    case Type::ACCESS_RESPONSE: {
        ret = recv_handler_caller<AccessResponse>(recv_buffer, header, sock, handlers.access_response, json);
        break;
    }
    case Type::LOCK_REQUEST: {
        ret = recv_handler_caller<LockRequest>(recv_buffer, header, sock, handlers.lock_request, json);
        break;
    }
    case Type::LOCK_RESPONSE: {
        ret = recv_handler_caller<LockResponse>(recv_buffer, header, sock, handlers.lock_response, json);
        break;
    }
    case Type::FLOCK_REQUEST: {
        ret = recv_handler_caller<FlockRequest>(recv_buffer, header, sock, handlers.flock_request, json);
        break;
    }
    case Type::FLOCK_RESPONSE: {
        ret = recv_handler_caller<FlockResponse>(recv_buffer, header, sock, handlers.flock_response, json);
        break;
    }
    case Type::FALLOCATE_REQUEST: {
        ret = recv_handler_caller<FallocateRequest>(recv_buffer, header, sock, handlers.fallocate_request, json);
        break;
    }
    case Type::FALLOCATE_RESPONSE: {
        ret = recv_handler_caller<FallocateResponse>(recv_buffer, header, sock, handlers.fallocate_response, json);
        break;
    }
    case Type::LSEEK_REQUEST: {
        ret = recv_handler_caller<LseekRequest>(recv_buffer, header, sock, handlers.lseek_request, json);
        break;
    }
    case Type::LSEEK_RESPONSE: {
        ret = recv_handler_caller<LseekResponse>(recv_buffer, header, sock, handlers.lseek_response, json);
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
    return ret;
};

int event_handle_recv(int sock, Header *header, char *recv_buffer, event_handlers &handlers, bool json) {
    int ret = -2;
    switch (header->type) {
    case Type::CURSOR_POSITION_EVENT: {
        ret = recv_handler_caller<CursorPosition>(recv_buffer, header, sock, handlers.cursor_position, json);
        break;
    }
    case Type::DIFF_WRITE_ENABLE_EVENT: {
        ret = recv_handler_caller<DiffWriteEnable>(recv_buffer, header, sock, handlers.diff_write_enable, json);
        break;
    }
    case Type::DIFF_WRITE_DISABLE_EVENT: {
        ret = recv_handler_caller<DiffWriteDisable>(recv_buffer, header, sock, handlers.diff_write_disable, json);
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
    return ret;
}
