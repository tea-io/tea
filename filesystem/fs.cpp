#include "fs.h"
#include "../common/log.h"
#include "../proto/messages.pb.h"
#include "tcp.h"
#include <sys/stat.h>

int sock;
config cfg;

static void *init(struct fuse_conn_info *conn, struct fuse_config *f_cfg) {
    (void)conn;
    (void)f_cfg;
    InitRequest req = InitRequest();
    req.set_name(cfg.name);
    InitResponse res;
    int err = request_response<InitResponse>(sock, req, &res, INIT_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return NULL;
    } else {
        log(INFO, sock, "The file system was initiated", res.error());
    }
    return NULL;
};

static void destroy(void *private_data) {
    (void)private_data;
    google::protobuf::ShutdownProtobufLibrary();
    close(sock);
};

static int get_attr_request(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void)fi;
    GetAttrRequest req = GetAttrRequest();
    req.set_path(path);
    GetAttrResponse res;
    int err = request_response<GetAttrResponse>(sock, req, &res, GET_ATTR_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -ENONET;
    }
    if (res.error() != 0) {
        return -res.error();
    }

    memset(stbuf, 0, sizeof(struct stat));
    stbuf->st_mode = res.mode();
    stbuf->st_size = res.size();
    stbuf->st_nlink = res.nlink();
    stbuf->st_atime = res.atime();
    stbuf->st_mtime = res.mtime();
    stbuf->st_ctime = res.ctime();
    if (res.own()) {
        stbuf->st_uid = getuid();
    }
    if (res.gown()) {
        stbuf->st_gid = getgid();
    }
    return -res.error();
};

static int open_fs(const char *path, struct fuse_file_info *fi) {
    OpenRequest req = OpenRequest();
    req.set_path(path);
    req.set_flags(fi->flags);
    OpenResponse res;
    int err = request_response<OpenResponse>(sock, req, &res, OPEN_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to open file: %d", res.error());
    }
    return -res.error();
};

static int release_fs(const char *path, struct fuse_file_info *fi) {
    (void)fi;
    ReleaseRequest req = ReleaseRequest();
    req.set_path(path);
    ReleaseResponse res;
    int err = request_response<ReleaseResponse>(sock, req, &res, RELEASE_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to release file: %d", res.error());
    }
    return -res.error();
};

static int readdir_fs(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void)fi;
    (void)flags;
    (void)offset;
    ReadDirRequest req = ReadDirRequest();
    req.set_path(path);
    ReadDirResponse res;
    int err = request_response<ReadDirResponse>(sock, req, &res, READ_DIR_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to read directory: %d", res.error());
    }
    for (int i = 0; i < res.names_size(); i++) {
        filler(buf, res.names(i).c_str(), NULL, 0, FUSE_FILL_DIR_PLUS);
    }
    return 0;
};

static int read_fs(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void)fi;
    ReadRequest req = ReadRequest();
    req.set_path(path);
    req.set_size(size);
    req.set_offset(offset);
    ReadResponse res;
    int err = request_response<ReadResponse>(sock, req, &res, READ_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to read file: %d", res.error());
    }
    if (res.error() != 0) {
        return -res.error();
    }
    memcpy(buf, res.data().c_str(), res.data().size());
    return res.data().size();
};

static int write_fs(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void)fi;
    WriteRequest req = WriteRequest();
    req.set_path(path);
    req.set_offset(offset);
    req.set_data(buf, size);
    WriteResponse res;
    int err = request_response<WriteResponse>(sock, req, &res, WRITE_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to write file: %d", res.error());
    }
    if (res.error() != 0) {
        return -res.error();
    }
    if (static_cast<long unsigned int>(res.size()) != size) {
        return -1;
    }
    return res.size();
};

static int create_fs(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void)fi;
    CreateRequest req = CreateRequest();
    req.set_path(path);
    req.set_mode(mode);
    CreateResponse res;
    int err = request_response<CreateResponse>(sock, req, &res, CREATE_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to create file: %d", res.error());
    }
    return -res.error();
};

static int mkdir_fs(const char *path, mode_t mode) {
    MkdirRequest req = MkdirRequest();
    req.set_path(path);
    req.set_mode(mode | S_IFDIR);
    MkdirResponse res;
    int err = request_response<MkdirResponse>(sock, req, &res, MKDIR_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to create directory: %d", res.error());
    }
    return -res.error();
};

static int unlink_fs(const char *path) {
    UnlinkRequest req = UnlinkRequest();
    req.set_path(path);
    UnlinkResponse res;
    int err = request_response<UnlinkResponse>(sock, req, &res, UNLINK_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to unlink: %d", res.error());
    }
    return -res.error();
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
fuse_operations get_fuse_operations(int sock_fd, config cfg_param) {
    sock = sock_fd;
    cfg = cfg_param;
    fuse_operations ops = {
        .getattr = get_attr_request,
        .mkdir = mkdir_fs,
        .unlink = unlink_fs,
        .open = open_fs,
        .read = read_fs,
        .write = write_fs,
        .release = release_fs,
        .readdir = readdir_fs,
        .init = init,
        .destroy = destroy,
        .create = create_fs,
    };
    return ops;
};
#pragma GCC diagnostic pop
