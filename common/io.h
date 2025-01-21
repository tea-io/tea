#pragma once
#include "../proto/messages.pb.h"
#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <google/protobuf/message.h>
#include <openssl/crypto.h>

int send_message(int sock, SSL *ssl, int id, Type type, google::protobuf::Message *message);

struct recv_handlers {
    int (*init_request)(int sock, SSL *ssl, int id, InitRequest *request);
    int (*init_response)(int sock, SSL *ssl, int id, InitResponse *response);
    int (*get_attr_request)(int sock, SSL *ssl, int id, GetAttrRequest *request);
    int (*get_attr_response)(int sock, SSL *ssl, int id, GetAttrResponse *response);
    int (*open_request)(int sock, SSL *ssl, int id, OpenRequest *request);
    int (*open_response)(int sock, SSL *ssl, int id, OpenResponse *response);
    int (*release_request)(int sock, SSL *ssl, int id, ReleaseRequest *request);
    int (*release_response)(int sock, SSL *ssl, int id, ReleaseResponse *response);
    int (*read_dir_request)(int sock, SSL *ssl, int id, ReadDirRequest *request);
    int (*read_dir_response)(int sock, SSL *ssl, int id, ReadDirResponse *response);
    int (*read_request)(int sock, SSL *ssl, int id, ReadRequest *request);
    int (*read_response)(int sock, SSL *ssl, int id, ReadResponse *response);
    int (*write_request)(int sock, SSL *ssl, int id, WriteRequest *request);
    int (*write_response)(int sock, SSL *ssl, int id, WriteResponse *response);
    int (*create_request)(int sock, SSL *ssl, int id, CreateRequest *request);
    int (*create_response)(int sock, SSL *ssl, int id, CreateResponse *response);
    int (*mkdir_request)(int sock, SSL *ssl, int id, MkdirRequest *request);
    int (*mkdir_response)(int sock, SSL *ssl, int id, MkdirResponse *response);
    int (*unlink_request)(int sock, SSL *ssl, int id, UnlinkRequest *request);
    int (*unlink_response)(int sock, SSL *ssl, int id, UnlinkResponse *response);
    int (*rmdir_request)(int sock, SSL *ssl, int id, RmdirRequest *request);
    int (*rmdir_response)(int sock, SSL *ssl, int id, RmdirResponse *response);
    int (*rename_request)(int sock, SSL *ssl, int id, RenameRequest *request);
    int (*rename_response)(int sock, SSL *ssl, int id, RenameResponse *response);
    int (*chmod_request)(int sock, SSL *ssl, int id, ChmodRequest *request);
    int (*chmod_response)(int sock, SSL *ssl, int id, ChmodResponse *response);
    int (*truncate_request)(int sock, SSL *ssl, int id, TruncateRequest *request);
    int (*truncate_response)(int sock, SSL *ssl, int id, TruncateResponse *response);
    int (*mknod_request)(int sock, SSL *ssl, int id, MknodRequest *request);
    int (*mknod_response)(int sock, SSL *ssl, int id, MknodResponse *response);
    int (*link_request)(int sock, SSL *ssl, int id, LinkRequest *request);
    int (*link_response)(int sock, SSL *ssl, int id, LinkResponse *response);
    int (*symlink_request)(int sock, SSL *ssl, int id, SymlinkRequest *request);
    int (*symlink_response)(int sock, SSL *ssl, int id, SymlinkResponse *response);
    int (*read_link_request)(int sock, SSL *ssl, int id, ReadLinkRequest *request);
    int (*read_link_response)(int sock, SSL *ssl, int id, ReadLinkResponse *response);
    int (*statfs_request)(int sock, SSL *ssl, int id, StatfsRequest *request);
    int (*statfs_response)(int sock, SSL *ssl, int id, StatfsResponse *response);
    int (*fsync_request)(int sock, SSL *ssl, int id, FsyncRequest *request);
    int (*fsync_response)(int sock, SSL *ssl, int id, FsyncResponse *response);
    int (*setxattr_request)(int sock, SSL *ssl, int id, SetxattrRequest *request);
    int (*setxattr_response)(int sock, SSL *ssl, int id, SetxattrResponse *response);
    int (*getxattr_request)(int sock, SSL *ssl, int id, GetxattrRequest *request);
    int (*getxattr_response)(int sock, SSL *ssl, int id, GetxattrResponse *response);
    int (*listxattr_request)(int sock, SSL *ssl, int id, ListxattrRequest *request);
    int (*listxattr_response)(int sock, SSL *ssl, int id, ListxattrResponse *response);
    int (*removexattr_request)(int sock, SSL *ssl, int id, RemovexattrRequest *request);
    int (*removexattr_response)(int sock, SSL *ssl, int id, RemovexattrResponse *response);
    int (*opendir_request)(int sock, SSL *ssl, int id, OpendirRequest *request);
    int (*opendir_response)(int sock, SSL *ssl, int id, OpendirResponse *response);
    int (*releasedir_request)(int sock, SSL *ssl, int id, ReleasedirRequest *request);
    int (*releasedir_response)(int sock, SSL *ssl, int id, ReleasedirResponse *response);
    int (*fsyncdir_request)(int sock, SSL *ssl, int id, FsyncdirRequest *request);
    int (*fsyncdir_response)(int sock, SSL *ssl, int id, FsyncdirResponse *response);
    int (*utimens_request)(int sock, SSL *ssl, int id, UtimensRequest *request);
    int (*utimens_response)(int sock, SSL *ssl, int id, UtimensResponse *response);
    int (*access_request)(int sock, SSL *ssl, int id, AccessRequest *request);
    int (*access_response)(int sock, SSL *ssl, int id, AccessResponse *response);
    int (*lock_request)(int sock, SSL *ssl, int id, LockRequest *request);
    int (*lock_response)(int sock, SSL *ssl, int id, LockResponse *response);
    int (*flock_request)(int sock, SSL *ssl, int id, FlockRequest *request);
    int (*flock_response)(int sock, SSL *ssl, int id, FlockResponse *response);
    int (*fallocate_request)(int sock, SSL *ssl, int id, FallocateRequest *request);
    int (*fallocate_response)(int sock, SSL *ssl, int id, FallocateResponse *response);
    int (*lseek_request)(int sock, SSL *ssl, int id, LseekRequest *request);
    int (*lseek_response)(int sock, SSL *ssl, int id, LseekResponse *response);
    int (*lsp_request)(int sock, SSL *ssl, int id, LspRequest *request);
    int (*lsp_response)(int sock, SSL *ssl, int id, LspResponse *response);
};

int handle_recv(int sock, SSL *ssl, recv_handlers &handlers);
int handle_recv_lsp(int sock, SSL *ssl, int server_sock, const std::function<int(int, SSL *, int, int, char *)> &handler);

int full_read(int fd, SSL *ssl, char &buf, int size);
int full_read(int fd, char &buf, int size);
