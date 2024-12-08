#pragma once
#include "../proto/messages.pb.h"
#include "header.h"

enum socket_type { FS, EVENT };

struct fs_handlers {
    int (*init_request)(int sock, int id, InitRequest *request);
    int (*init_response)(int sock, int id, InitResponse *response);
    int (*get_attr_request)(int sock, int id, GetAttrRequest *request);
    int (*get_attr_response)(int sock, int id, GetAttrResponse *response);
    int (*open_request)(int sock, int id, OpenRequest *request);
    int (*open_response)(int sock, int id, OpenResponse *response);
    int (*release_request)(int sock, int id, ReleaseRequest *request);
    int (*release_response)(int sock, int id, ReleaseResponse *response);
    int (*read_dir_request)(int sock, int id, ReadDirRequest *request);
    int (*read_dir_response)(int sock, int id, ReadDirResponse *response);
    int (*read_request)(int sock, int id, ReadRequest *request);
    int (*read_response)(int sock, int id, ReadResponse *response);
    int (*write_request)(int sock, int id, WriteRequest *request);
    int (*write_response)(int sock, int id, WriteResponse *response);
    int (*create_request)(int sock, int id, CreateRequest *request);
    int (*create_response)(int sock, int id, CreateResponse *response);
    int (*mkdir_request)(int sock, int id, MkdirRequest *request);
    int (*mkdir_response)(int sock, int id, MkdirResponse *response);
    int (*unlink_request)(int sock, int id, UnlinkRequest *request);
    int (*unlink_response)(int sock, int id, UnlinkResponse *response);
    int (*rmdir_request)(int sock, int id, RmdirRequest *request);
    int (*rmdir_response)(int sock, int id, RmdirResponse *response);
    int (*rename_request)(int sock, int id, RenameRequest *request);
    int (*rename_response)(int sock, int id, RenameResponse *response);
    int (*chmod_request)(int sock, int id, ChmodRequest *request);
    int (*chmod_response)(int sock, int id, ChmodResponse *response);
    int (*truncate_request)(int sock, int id, TruncateRequest *request);
    int (*truncate_response)(int sock, int id, TruncateResponse *response);
    int (*mknod_request)(int sock, int id, MknodRequest *request);
    int (*mknod_response)(int sock, int id, MknodResponse *response);
    int (*link_request)(int sock, int id, LinkRequest *request);
    int (*link_response)(int sock, int id, LinkResponse *response);
    int (*symlink_request)(int sock, int id, SymlinkRequest *request);
    int (*symlink_response)(int sock, int id, SymlinkResponse *response);
    int (*read_link_request)(int sock, int id, ReadLinkRequest *request);
    int (*read_link_response)(int sock, int id, ReadLinkResponse *response);
    int (*statfs_request)(int sock, int id, StatfsRequest *request);
    int (*statfs_response)(int sock, int id, StatfsResponse *response);
    int (*fsync_request)(int sock, int id, FsyncRequest *request);
    int (*fsync_response)(int sock, int id, FsyncResponse *response);
    int (*setxattr_request)(int sock, int id, SetxattrRequest *request);
    int (*setxattr_response)(int sock, int id, SetxattrResponse *response);
    int (*getxattr_request)(int sock, int id, GetxattrRequest *request);
    int (*getxattr_response)(int sock, int id, GetxattrResponse *response);
    int (*listxattr_request)(int sock, int id, ListxattrRequest *request);
    int (*listxattr_response)(int sock, int id, ListxattrResponse *response);
    int (*removexattr_request)(int sock, int id, RemovexattrRequest *request);
    int (*removexattr_response)(int sock, int id, RemovexattrResponse *response);
    int (*opendir_request)(int sock, int id, OpendirRequest *request);
    int (*opendir_response)(int sock, int id, OpendirResponse *response);
    int (*releasedir_request)(int sock, int id, ReleasedirRequest *request);
    int (*releasedir_response)(int sock, int id, ReleasedirResponse *response);
    int (*fsyncdir_request)(int sock, int id, FsyncdirRequest *request);
    int (*fsyncdir_response)(int sock, int id, FsyncdirResponse *response);
    int (*utimens_request)(int sock, int id, UtimensRequest *request);
    int (*utimens_response)(int sock, int id, UtimensResponse *response);
    int (*access_request)(int sock, int id, AccessRequest *request);
    int (*access_response)(int sock, int id, AccessResponse *response);
    int (*lock_request)(int sock, int id, LockRequest *request);
    int (*lock_response)(int sock, int id, LockResponse *response);
    int (*flock_request)(int sock, int id, FlockRequest *request);
    int (*flock_response)(int sock, int id, FlockResponse *response);
    int (*fallocate_request)(int sock, int id, FallocateRequest *request);
    int (*fallocate_response)(int sock, int id, FallocateResponse *response);
    int (*lseek_request)(int sock, int id, LseekRequest *request);
    int (*lseek_response)(int sock, int id, LseekResponse *response);
};

struct event_handlers {
    int (*cursor_position)(int sock, int id, CursorPosition *request);
    int (*diff_write_enable)(int sock, int id, DiffWriteEnable *request);
    int (*diff_write_disable)(int sock, int id, DiffWriteDisable *request);
};

union recv_handlers {
    fs_handlers fs;
    event_handlers event;
};

int event_handle_recv(int sock, Header *header, char *recv_buffer, event_handlers &handlers, bool json);
int fs_handle_recv(int sock, Header *header, char *recv_buffer, fs_handlers &handlers, bool json);
