#include "fs.h"

#include "../common/log.h"
#include "../proto/messages.pb.h"
#include "lsp.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <linux/limits.h>
#include <list>
#include <string>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
#include <sys/xattr.h>
#include <unistd.h>

struct client_info {
    int fd;
    std::string name;
};

std::list<client_info> clients_info;
std::string base_path = "";
std::map<int, DIR *> dirs;
long int dir_iter = 0;

static int init_request(int sock, SSL *ssl, int id, InitRequest *req) {
    clients_info.push_back(client_info{.fd = sock, .name = req->name()});
    (void)req;
    InitResponse res;
    res.set_error(0);
    int err = send_message(sock, ssl, id, Type::INIT_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int get_attr_request(int sock, SSL *ssl, int id, GetAttrRequest *req) {
    GetAttrResponse res;
    // check if the path is inside the base path (prevent directory traversal)
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EPERM);
    } else {
        struct stat st;
        // weekly_cannonical don't work with symlinks
        std::string raw_path = base_path + req->path();
        int err = lstat(raw_path.c_str(), &st);
        if (err < 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
            res.set_mode(st.st_mode);
            res.set_size(st.st_size);
            res.set_nlink(st.st_nlink);
            res.set_atime(st.st_atime);
            res.set_mtime(st.st_mtime);
            res.set_ctime(st.st_ctime);
            res.set_own(getuid() == st.st_uid);
            res.set_gown(getgid() == st.st_gid);
        }
    }
    int err = send_message(sock, ssl, id, Type::GET_ATTR_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int open_request(int sock, SSL *ssl, int id, OpenRequest *req) {
    OpenResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        int fd = open(path.c_str(), req->flags());
        if (fd > 0) {
            res.set_fd(fd);
        } else {
            res.set_error(errno);
        }
    }

    int err = send_message(sock, ssl, id, Type::OPEN_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int release_request(int sock, SSL *ssl, int id, ReleaseRequest *req) {
    (void)req;
    ReleaseResponse res;
    int err = close(req->fd());
    if (err < 0) {
        res.set_error(errno);
    } else {
        res.set_error(0);
    }
    err = send_message(sock, ssl, id, Type::RELEASE_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int read_dir_request(int sock, SSL *ssl, int id, ReadDirRequest *req) {
    ReadDirResponse res;
    DIR *dir = dirs[req->directory_descriptor()];
    if (dir == nullptr) {
        res.set_error(errno);
    } else {
        struct dirent *entry;
        errno = 0;
        while ((entry = readdir(dir)) != nullptr) {
            res.add_names(entry->d_name);
        }
        if (errno != 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
        }
    }
    int err = send_message(sock, ssl, id, Type::READ_DIR_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int read_request(int sock, SSL *ssl, int id, ReadRequest *req) {
    ReadResponse res;
    char *buf = new char[req->size()];
    int err = lseek(req->fd(), req->offset(), SEEK_SET);
    if (err >= 0) {
        err = read(req->fd(), buf, req->size());
    }
    if (err < 0) {
        res.set_error(errno);
    } else {
        res.set_error(0);
        res.set_data(buf, err);
    }
    err = send_message(sock, ssl, id, Type::READ_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int write_request(int sock, SSL *ssl, int id, WriteRequest *req) {
    WriteResponse res;
    int err = lseek(req->fd(), req->offset(), SEEK_SET);
    if (err >= 0) {
        err = write(req->fd(), req->data().c_str(), req->data().size());
    }
    if (err < 0) {
        res.set_error(errno);
    } else {
        res.set_error(0);
        res.set_size(err);
    }
    err = send_message(sock, ssl, id, Type::WRITE_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int create_request(int sock, SSL *ssl, int id, CreateRequest *req) {
    CreateResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        int fd = creat(path.c_str(), req->mode());
        if (fd > 0) {
            res.set_fd(fd);
        } else {
            res.set_error(errno);
        }
    }

    int err = send_message(sock, ssl, id, Type::CREATE_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int mkdir_request(int sock, SSL *ssl, int id, MkdirRequest *req) {
    MkdirResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        int err = mkdir(path.c_str(), req->mode());
        if (err < 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
        }
    }
    int err = send_message(sock, ssl, id, Type::MKDIR_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int unlink_request(int sock, SSL *ssl, int id, UnlinkRequest *req) {
    UnlinkResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        std::string raw_path = base_path + req->path();
        int err = unlink(raw_path.c_str());
        if (err < 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
        }
    }
    int err = send_message(sock, ssl, id, Type::UNLINK_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int rmdir_request(int sock, SSL *ssl, int id, RmdirRequest *req) {
    RmdirResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        int err = rmdir(path.c_str());
        if (err < 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
        }
    }
    int err = send_message(sock, ssl, id, Type::RMDIR_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int rename_normal(std::string old_path, std::string new_path) {
    int err = rename(old_path.c_str(), new_path.c_str());
    if (err < 0) {
        return errno;
    }
    return 0;
}

static int rename_no_replace(std::string old_path, std::string new_path) {
    if (std::filesystem::exists(new_path)) {
        return EEXIST;
    }
    int err = rename(old_path.c_str(), new_path.c_str());
    if (err < 0) {
        return errno;
    }
    return 0;
}

static int rename_exchange(std::string old_path, std::string new_path) {
    if (!std::filesystem::exists(new_path) || !std::filesystem::exists(old_path)) {
        return ENOENT;
    }
    int err = syscall(SYS_renameat2, AT_FDCWD, old_path.c_str(), AT_FDCWD, new_path.c_str(), RENAME_EXCHANGE);
    if (err < 0) {
        return errno;
    }
    return 0;
}

static int rename_request(int sock, SSL *ssl, int id, RenameRequest *req) {
    RenameResponse res;
    std::string new_path = std::filesystem::weakly_canonical(base_path + req->new_path());
    if (new_path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    }
    std::string old_path = std::filesystem::weakly_canonical(base_path + req->old_path());
    if (old_path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    }
    if (res.error() == 0) {
        int flags = req->flags();
        int err = EINVAL;
        if (flags == 0) {
            err = rename_normal(old_path, new_path);
        } else if (flags == RENAME_NOREPLACE) {
            err = rename_no_replace(old_path, new_path);
        } else if (flags == RENAME_EXCHANGE) {
            err = rename_exchange(old_path, new_path);
        }
        res.set_error(err);
    }
    int err = send_message(sock, ssl, id, Type::RENAME_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int chmod_request(int sock, SSL *ssl, int id, ChmodRequest *req) {
    ChmodResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        int err = chmod(path.c_str(), req->mode());
        if (err < 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
        }
    }
    int err = send_message(sock, ssl, id, Type::CHMOD_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int truncate_request(int sock, SSL *ssl, int id, TruncateRequest *req) {
    TruncateResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        int err = truncate(path.c_str(), req->size());
        if (err < 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
        }
    }
    int err = send_message(sock, ssl, id, Type::TRUNCATE_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int mknod_request(int sock, SSL *ssl, int id, MknodRequest *req) {
    MknodResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        int err = mknod(path.c_str(), req->mode(), req->dev());
        if (err < 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
        }
    }
    int err = send_message(sock, ssl, id, Type::MKNOD_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int link_request(int sock, SSL *ssl, int id, LinkRequest *req) {
    LinkResponse res;
    std::string old_path = std::filesystem::weakly_canonical(base_path + req->old_path());
    std::string new_path = std::filesystem::weakly_canonical(base_path + req->new_path());
    if (old_path.substr(0, base_path.size()) != base_path || new_path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        int err = link(old_path.c_str(), new_path.c_str());
        if (err < 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
        }
    }
    int err = send_message(sock, ssl, id, Type::LINK_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int symlink_request(int sock, SSL *ssl, int id, SymlinkRequest *req) {
    SymlinkResponse res;
    std::string old_path = std::filesystem::weakly_canonical(base_path + req->old_path());
    std::string new_path = std::filesystem::weakly_canonical(base_path + req->new_path());
    if (old_path.substr(0, base_path.size()) != base_path || new_path.substr(0, base_path.size()) != base_path) {
        res.set_error(EXDEV);
    } else {
        int err = symlink(old_path.c_str(), new_path.c_str());
        if (err < 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
        }
    }
    int err = send_message(sock, ssl, id, Type::SYMLINK_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int read_link_request(int sock, SSL *ssl, int id, ReadLinkRequest *req) {
    ReadLinkResponse res;
    char buf[100];
    std::string path = base_path + req->path().c_str();
    int err = readlink(path.c_str(), buf, 100);
    if (err < 0) {
        res.set_error(errno);
    } else {
        std::string link = buf;
        if (link.substr(0, base_path.size()) != base_path) {
            res.set_error(EACCES);
        } else {
            link = link.substr(base_path.size() + 1, err);
            res.set_error(0);
            res.set_path(link);
        }
    }
    err = send_message(sock, ssl, id, Type::READ_LINK_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int statfs_request(int sock, SSL *ssl, int id, StatfsRequest *req) {
    StatfsResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        struct statvfs st;
        int err = statvfs(path.c_str(), &st);
        if (err < 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
            res.set_frsize(st.f_frsize);
            res.set_blocks(st.f_blocks);
            res.set_bfree(st.f_bfree);
            res.set_bavail(st.f_bavail);
            res.set_bsize(st.f_bsize);
            res.set_files(st.f_files);
            res.set_ffree(st.f_ffree);
            res.set_namemax(st.f_namemax);
            res.set_type(st.f_type);
        }
    }
    int err = send_message(sock, ssl, id, Type::STATFS_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int fsync_request(int sock, SSL *ssl, int id, FsyncRequest *req) {
    FsyncResponse res;
    int err = fsync(req->fd());
    if (err < 0) {
        res.set_error(errno);
    } else {
        res.set_error(0);
    }
    err = send_message(sock, ssl, id, Type::FSYNC_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int setxattr_request(int sock, SSL *ssl, int id, SetxattrRequest *req) {
    SetxattrResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        int err = setxattr(path.c_str(), req->name().c_str(), req->value().c_str(), req->value().size(), req->flags());
        if (err < 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
        }
    }
    int err = send_message(sock, ssl, id, Type::SETXATTR_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int getxattr_request(int sock, SSL *ssl, int id, GetxattrRequest *req) {
    GetxattrResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        char buf[100];
        int err = getxattr(path.c_str(), req->name().c_str(), buf, 100);
        if (err < 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
            res.set_value(buf, err);
        }
    }
    int err = send_message(sock, ssl, id, Type::GETXATTR_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int listxattr_request(int sock, SSL *ssl, int id, ListxattrRequest *req) {
    ListxattrResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        char buf[300];
        int err = listxattr(path.c_str(), buf, 300);
        if (err < 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
            char *curr = buf;
            int curr_len = 0;
            std::string name;
            while (curr_len < err) {
                int len = strlen(curr);
                name.assign(curr, curr + len);
                res.add_names(name);
                curr += len + 1;
                curr_len += len + 1;
            }
        }
    }
    int err = send_message(sock, ssl, id, Type::LISTXATTR_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int removexattr_request(int sock, SSL *ssl, int id, RemovexattrRequest *req) {
    RemovexattrResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        int err = removexattr(path.c_str(), req->name().c_str());
        if (err < 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
        }
    }
    int err = send_message(sock, ssl, id, Type::REMOVEXATTR_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int opendir_request(int sock, SSL *ssl, int id, OpendirRequest *req) {
    OpendirResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        DIR *dir = opendir(path.c_str());
        if (dir == nullptr) {
            res.set_error(errno);
        } else {
            res.set_error(0);
            dirs[dir_iter] = dir;
            res.set_directory_descriptor(dir_iter++);
        }
    }
    int err = send_message(sock, ssl, id, Type::OPENDIR_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int releasedir_fs(int sock, SSL *ssl, int id, ReleasedirRequest *req) {
    (void)req;
    ReleasedirResponse res;
    DIR *dir = dirs[req->directory_descriptor()];
    int err;
    if (dir == nullptr) {
        res.set_error(ENOENT);
    } else {
        dirs.erase(req->directory_descriptor());
        err = closedir(dir);
    }
    if (err < 0) {
        res.set_error(errno);
    } else {
        res.set_error(0);
    }
    err = send_message(sock, ssl, id, Type::RELEASEDIR_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int fsyncdir_request(int sock, SSL *ssl, int id, FsyncdirRequest *req) {
    FsyncResponse res;
    DIR *dir = dirs[req->directory_descriptor()];
    int fd = dirfd(dir);
    int err = fsync(fd);
    if (err < 0) {
        res.set_error(errno);
    } else {
        res.set_error(0);
    }
    err = send_message(sock, ssl, id, Type::FSYNC_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int utimens_fs(int sock, SSL *ssl, int id, UtimensRequest *req) {
    UtimensResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        struct timespec times[2];
        times[0].tv_sec = req->atime();
        times[0].tv_nsec = 0;
        times[1].tv_sec = req->mtime();
        times[1].tv_nsec = 0;
        int err = utimensat(AT_FDCWD, path.c_str(), times, 0);
        if (err < 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
        }
    }
    int err = send_message(sock, ssl, id, Type::UTIMENS_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int access_request(int sock, SSL *ssl, int id, AccessRequest *req) {
    AccessResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        int err = access(path.c_str(), req->mode());
        if (err < 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
        }
    }
    int err = send_message(sock, ssl, id, Type::ACCESS_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int lock_request(int sock, SSL *ssl, int id, LockRequest *req) {
    LockResponse res;
    res.set_error(EACCES);
    struct flock lock;
    lock.l_type = req->lock().l_type();
    lock.l_whence = req->lock().l_whence();
    lock.l_start = req->lock().l_start();
    lock.l_len = req->lock().l_len();
    int err = fcntl(req->fd(), req->cmd(), &lock);
    if (err < 0) {
        res.set_error(errno);
    } else {
        res.set_error(0);
    }
    if (req->cmd() == F_GETLK) {
        Lock *l = res.mutable_lock();
        l->set_l_type(lock.l_type);
        l->set_l_whence(lock.l_whence);
        l->set_l_start(lock.l_start);
        l->set_l_len(lock.l_len);
    }
    err = send_message(sock, ssl, id, Type::LOCK_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int flock_request(int sock, SSL *ssl, int id, FlockRequest *req) {
    FlockResponse res;
    int err = flock(req->fd(), req->op());
    if (err < 0) {
        res.set_error(errno);
    } else {
        res.set_error(0);
    }
    err = send_message(sock, ssl, id, Type::FLOCK_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int fallocate_request(int sock, SSL *ssl, int id, FallocateRequest *req) {
    FallocateResponse res;
    int err = fallocate(req->fd(), req->mode(), req->offset(), req->len());
    if (err < 0) {
        res.set_error(errno);
    } else {
        res.set_error(0);
    }
    err = send_message(sock, ssl, id, Type::FALLOCATE_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int lseek_request(int sock, SSL *ssl, int id, LseekRequest *req) {
    LseekResponse res;
    off_t off = lseek(req->fd(), req->offset(), req->whence());
    if (off < 0) {
        res.set_error(errno);
    } else {
        res.set_error(0);
        res.set_offset(off);
    }
    int err = send_message(sock, ssl, id, Type::LSEEK_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

template <typename T> int respons_handler(int sock, SSL *ssl, int id, T message) {
    (void)sock;
    (void)ssl;
    (void)id;
    (void)message;
    return 0;
}

recv_handlers get_handlers(std::string path) {
    base_path = std::filesystem::weakly_canonical(path);
    return recv_handlers{
        .init_request = init_request,
        .init_response = respons_handler<InitResponse *>,
        .get_attr_request = get_attr_request,
        .get_attr_response = respons_handler<GetAttrResponse *>,
        .open_request = open_request,
        .open_response = respons_handler<OpenResponse *>,
        .release_request = release_request,
        .release_response = respons_handler<ReleaseResponse *>,
        .read_dir_request = read_dir_request,
        .read_dir_response = respons_handler<ReadDirResponse *>,
        .read_request = read_request,
        .read_response = respons_handler<ReadResponse *>,
        .write_request = write_request,
        .write_response = respons_handler<WriteResponse *>,
        .create_request = create_request,
        .create_response = respons_handler<CreateResponse *>,
        .mkdir_request = mkdir_request,
        .mkdir_response = respons_handler<MkdirResponse *>,
        .unlink_request = unlink_request,
        .unlink_response = respons_handler<UnlinkResponse *>,
        .rmdir_request = rmdir_request,
        .rmdir_response = respons_handler<RmdirResponse *>,
        .rename_request = rename_request,
        .rename_response = respons_handler<RenameResponse *>,
        .chmod_request = chmod_request,
        .chmod_response = respons_handler<ChmodResponse *>,
        .truncate_request = truncate_request,
        .truncate_response = respons_handler<TruncateResponse *>,
        .mknod_request = mknod_request,
        .mknod_response = respons_handler<MknodResponse *>,
        .link_request = link_request,
        .link_response = respons_handler<LinkResponse *>,
        .symlink_request = symlink_request,
        .symlink_response = respons_handler<SymlinkResponse *>,
        .read_link_request = read_link_request,
        .read_link_response = respons_handler<ReadLinkResponse *>,
        .statfs_request = statfs_request,
        .statfs_response = respons_handler<StatfsResponse *>,
        .fsync_request = fsync_request,
        .fsync_response = respons_handler<FsyncResponse *>,
        .setxattr_request = setxattr_request,
        .setxattr_response = respons_handler<SetxattrResponse *>,
        .getxattr_request = getxattr_request,
        .getxattr_response = respons_handler<GetxattrResponse *>,
        .listxattr_request = listxattr_request,
        .listxattr_response = respons_handler<ListxattrResponse *>,
        .removexattr_request = removexattr_request,
        .removexattr_response = respons_handler<RemovexattrResponse *>,
        .opendir_request = opendir_request,
        .opendir_response = respons_handler<OpendirResponse *>,
        .releasedir_request = releasedir_fs,
        .releasedir_response = respons_handler<ReleasedirResponse *>,
        .fsyncdir_request = fsyncdir_request,
        .fsyncdir_response = respons_handler<FsyncdirResponse *>,
        .utimens_request = utimens_fs,
        .utimens_response = respons_handler<UtimensResponse *>,
        .access_request = access_request,
        .access_response = respons_handler<AccessResponse *>,
        .lock_request = lock_request,
        .lock_response = respons_handler<LockResponse *>,
        .flock_request = flock_request,
        .flock_response = respons_handler<FlockResponse *>,
        .fallocate_request = fallocate_request,
        .fallocate_response = respons_handler<FallocateResponse *>,
        .lseek_request = lseek_request,
        .lseek_response = respons_handler<LseekResponse *>,
        .lsp_request = handle_lsp_request,
        .lsp_response = respons_handler<LspResponse *>,
    };
}
