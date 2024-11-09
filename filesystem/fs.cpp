#include "fs.h"
#include "../common/log.h"
#include "../proto/messages.pb.h"
#include "tcp.h"
#include <cstring>
#include <sys/stat.h>
#include <sys/xattr.h>

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

static int rmdir_fs(const char *path) {
    RmdirRequest req = RmdirRequest();
    req.set_path(path);
    RmdirResponse res;
    int err = request_response<RmdirResponse>(sock, req, &res, RMDIR_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to remove directory: %d", res.error());
    }
    return -res.error();
}

static int rename_fs(const char *old_path, const char *new_path, unsigned int flags) {
    RenameRequest req = RenameRequest();
    req.set_old_path(old_path);
    req.set_new_path(new_path);
    req.set_flags(flags);
    RenameResponse res;
    int err = request_response<RenameResponse>(sock, req, &res, RENAME_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to rename: %d", res.error());
    }
    return -res.error();
}

static int chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi) {
    (void)path;
    (void)uid;
    (void)gid;
    (void)fi;
    // chown is not supported
    return EACCES;
}

static int chmod(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void)fi;
    ChmodRequest req = ChmodRequest();
    req.set_path(path);
    req.set_mode(mode);
    ChmodResponse res;
    int err = request_response<ChmodResponse>(sock, req, &res, CHMOD_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to change mode: %d", res.error());
    }
    log(DEBUG, sock, "Change mode: %d", res.error());
    return -res.error();
}

static int truncate_fs(const char *path, off_t size, struct fuse_file_info *fi) {
    (void)fi;
    TruncateRequest req = TruncateRequest();
    req.set_path(path);
    req.set_size(size);
    TruncateResponse res;
    int err = request_response<TruncateResponse>(sock, req, &res, TRUNCATE_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to truncate: %d", res.error());
    }
    return -res.error();
}

static int mknod_fs(const char *path, mode_t mode, dev_t dev) {
    MknodRequest req = MknodRequest();
    req.set_path(path);
    req.set_mode(mode);
    req.set_dev(dev);
    MknodResponse res;
    int err = request_response<MknodResponse>(sock, req, &res, MKNOD_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to create node: %d", res.error());
    }
    return -res.error();
}

static int link_fs(const char *old_path, const char *new_path) {
    LinkRequest req = LinkRequest();
    req.set_old_path(old_path);
    req.set_new_path(new_path);
    LinkResponse res;
    int err = request_response<LinkResponse>(sock, req, &res, LINK_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to link: %d", res.error());
    }
    return -res.error();
}

// symlink only works on relative paths inside the mount point
static int symlink_fs(const char *old_path, const char *new_path) {
    SymlinkRequest req = SymlinkRequest();
    std::string old_path_str = old_path;
    if (old_path[0] != '/') {
        old_path_str = "/" + old_path_str;
    }
    req.set_old_path(old_path_str);
    req.set_new_path(new_path);
    SymlinkResponse res;
    int err = request_response<SymlinkResponse>(sock, req, &res, SYMLINK_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to symlink: %d", res.error());
    }
    return -res.error();
}

static int read_link_fs(const char *link, char *buf, size_t s) {
    ReadLinkRequest req = ReadLinkRequest();
    req.set_path(link);
    ReadLinkResponse res;
    int err = request_response<ReadLinkResponse>(sock, req, &res, READ_LINK_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to readlink: %d", res.error());
        std::string path = res.path();
        if (s < path.size()) {
            path = path.substr(0, s);
        }
        strcpy(buf, res.path().c_str());
    }
    return -res.error();
}

static int statfs(const char *path, struct statvfs *stbuf) {
    StatfsRequest req = StatfsRequest();
    req.set_path(path);
    StatfsResponse res;
    int err = request_response<StatfsResponse>(sock, req, &res, STATFS_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to statfs: %d", res.error());
    }
    if (res.error() != 0) {
        return -res.error();
    }
    stbuf->f_bsize = res.bsize();
    stbuf->f_frsize = res.frsize();
    stbuf->f_blocks = res.blocks();
    stbuf->f_bfree = res.bfree();
    stbuf->f_bavail = res.bavail();
    stbuf->f_files = res.files();
    stbuf->f_ffree = res.ffree();
    stbuf->f_namemax = res.namemax();
    return -res.error();
}

static int flush_fs(const char *path, struct fuse_file_info *fi) {
    (void)path;
    (void)fi;
    return 0;
};

static int fsync_fs(const char *path, int datasync, struct fuse_file_info *fi) {
    (void)datasync;
    (void)fi;
    FsyncRequest req = FsyncRequest();
    req.set_path(path);
    FsyncResponse res;
    int err = request_response<FsyncResponse>(sock, req, &res, FSYNC_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to fsync: %d", res.error());
    }
    return -res.error();
};

static int setxattr_fs(const char *path, const char *name, const char *value, size_t size, int flags) {
    SetxattrRequest req = SetxattrRequest();
    req.set_path(path);
    req.set_name(name);
    req.set_value(value, size);
    req.set_flags(flags);
    SetxattrResponse res;
    int err = request_response<SetxattrResponse>(sock, req, &res, SETXATTR_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to setxattr: %d", res.error());
    }
    return -res.error();
};

static int getxattr_fs(const char *path, const char *name, char *value, size_t size) {
    GetxattrRequest req = GetxattrRequest();
    req.set_path(path);
    req.set_name(name);
    GetxattrResponse res;
    int err = request_response<GetxattrResponse>(sock, req, &res, GETXATTR_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to getxattr: %d", res.error());
    }
    if (res.error() != 0) {
        return -res.error();
    }
    if (size < res.value().size()) {
        return -ERANGE;
    }
    memcpy(value, res.value().c_str(), res.value().size());
    return res.value().size();
};

static int listxattr_fs(const char *path, char *list, size_t size) {
    ListxattrRequest req = ListxattrRequest();
    req.set_path(path);
    ListxattrResponse res;
    int err = request_response<ListxattrResponse>(sock, req, &res, LISTXATTR_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to listxattr: %d", res.error());
    }
    if (res.error() != 0) {
        return -res.error();
    }
    char names[300];
    size_t total_size = 0;
    for (int i = 0; i < res.names_size(); i++) {
        for (size_t j = 0; j < res.names(i).size(); j++) {
            names[total_size] = res.names(i)[j];
            total_size++;
            if (total_size >= 300) {
                return -ERANGE;
            }
        }
        names[total_size] = '\0';
        total_size++;
        if (total_size >= 300) {
            return -ERANGE;
        }
    }
    if (size < total_size) {
        return -ERANGE;
    }
    memcpy(list, names, total_size);
    log(DEBUG, sock, "Listxattr size: %d", total_size);
    return total_size;
};

static int removexattr_fs(const char *path, const char *name) {
    RemovexattrRequest req = RemovexattrRequest();
    req.set_path(path);
    req.set_name(name);
    RemovexattrResponse res;
    int err = request_response<RemovexattrResponse>(sock, req, &res, REMOVEXATTR_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to removexattr: %d", res.error());
    }
    return -res.error();
};

static int opendir_fs(const char *path, fuse_file_info *fi) {
    (void)fi;
    OpendirRequest req = OpendirRequest();
    req.set_path(path);
    OpendirResponse res;
    int err = request_response<OpendirResponse>(sock, req, &res, OPENDIR_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to opendir: %d", res.error());
    }
    return -res.error();
};

static int releasedir_fs(const char *path, struct fuse_file_info *fi) {
    (void)fi;
    ReleasedirRequest req = ReleasedirRequest();
    req.set_path(path);
    ReleasedirResponse res;
    int err = request_response<ReleasedirResponse>(sock, req, &res, RELEASEDIR_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to releasedir: %d", res.error());
    }
    return -res.error();
};

static int fsyncdir_fs(const char *path, int datasync, struct fuse_file_info *fi) {
    (void)datasync;
    (void)fi;
    FsyncdirRequest req = FsyncdirRequest();
    req.set_path(path);
    FsyncdirResponse res;
    int err = request_response<FsyncdirResponse>(sock, req, &res, FSYNCDIR_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to fsyncdir: %d", res.error());
    }
    return -res.error();
};

static int utimens_fs(const char *path, const struct timespec tv[2], struct fuse_file_info *fi) {
    (void)fi;
    UtimensRequest req = UtimensRequest();
    req.set_path(path);
    req.set_atime(tv[0].tv_sec);
    req.set_mtime(tv[1].tv_sec);
    UtimensResponse res;
    int err = request_response<UtimensResponse>(sock, req, &res, UTIMENS_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    } else {
        log(INFO, sock, "Try to utimens: %d", res.error());
    }
    return -res.error();
};

static int access_fs(const char *path, int mask) {
    AccessRequest req = AccessRequest();
    req.set_path(path);
    req.set_mode(mask);
    AccessResponse res;
    int err = request_response<AccessResponse>(sock, req, &res, ACCESS_REQUEST);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return -1;
    }
    return -res.error();
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
fuse_operations get_fuse_operations(int sock_fd, config cfg_param) {
    sock = sock_fd;
    cfg = cfg_param;
    fuse_operations ops = {
        .getattr = get_attr_request,
        .readlink = read_link_fs,
        .mknod = mknod_fs,
        .mkdir = mkdir_fs,
        .unlink = unlink_fs,
        .rmdir = rmdir_fs,
        .symlink = symlink_fs,
        .rename = rename_fs,
        .link = link_fs,
        .chmod = chmod,
        .chown = chown,
        .truncate = truncate_fs,
        .open = open_fs,
        .read = read_fs,
        .write = write_fs,
        .statfs = statfs,
        .flush = flush_fs,
        .release = release_fs,
        .fsync = fsync_fs,
        .setxattr = setxattr_fs,
        .getxattr = getxattr_fs,
        .listxattr = listxattr_fs,
        .removexattr = removexattr_fs,
        .opendir = opendir_fs,
        .readdir = readdir_fs,
        .releasedir = releasedir_fs,
        .fsyncdir = fsyncdir_fs,
        .init = init,
        .destroy = destroy,
        .access = access_fs,
        .create = create_fs,
        .utimens = utimens_fs,
    };
    return ops;
};
#pragma GCC diagnostic pop
