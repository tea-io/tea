#include "fs.h"
#include "../common/log.h"
#include "../proto/messages.pb.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <list>
#include <string>
#include <sys/stat.h>
#include <sys/syscall.h>

struct client_info {
    int fd;
    std::string name;
};

std::list<client_info> clients_info;
std::string base_path = "";
std::map<std::string, int> fds;

static int init_request(int sock, int id, InitRequest *req) {
    clients_info.push_back(client_info{.fd = sock, .name = req->name()});
    (void)req;
    InitResponse res;
    res.set_error(0);
    int err = send_message(sock, id, Type::INIT_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int get_attr_request(int sock, int id, GetAttrRequest *req) {
    GetAttrResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        struct stat st;
        int err = stat(path.c_str(), &st);
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
    int err = send_message(sock, id, Type::GET_ATTR_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int open_request(int sock, int id, OpenRequest *req) {
    OpenResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        int fd = open(path.c_str(), req->flags());
        if (fd > 0) {
            fds[req->path()] = fd;
        } else {
            res.set_error(errno);
        }
    }

    int err = send_message(sock, id, Type::OPEN_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int release_request(int sock, int id, ReleaseRequest *req) {
    (void)req;
    ReleaseResponse res;
    int err = close(fds[req->path()]);
    if (err < 0) {
        res.set_error(errno);
    } else {
        res.set_error(0);
    }
    err = send_message(sock, id, Type::RELEASE_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int read_dir_request(int sock, int id, ReadDirRequest *req) {
    ReadDirResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        DIR *dir = opendir(path.c_str());
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
            closedir(dir);
        }
    }
    int err = send_message(sock, id, Type::READ_DIR_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int read_request(int sock, int id, ReadRequest *req) {
    ReadResponse res;
    int fd = fds[req->path()];
    if (fd < 0) {
        res.set_error(EBADF);
    }
    char *buf = new char[req->size()];
    int n = read(fd, buf, req->size());
    if (n < 0) {
        res.set_error(errno);
    } else {
        res.set_error(0);
        res.set_data(buf, n);
    }
    int err = send_message(sock, id, Type::READ_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int write_request(int sock, int id, WriteRequest *req) {
    WriteResponse res;
    int fd = fds[req->path()];
    if (fd < 0) {
        res.set_error(EBADF);
    }
    int n = write(fd, req->data().c_str(), req->data().size());
    log(ERROR, sock, "Write %d bytes", n);
    if (n < 0) {
        res.set_error(errno);
    } else {
        res.set_error(0);
        res.set_size(n);
    }
    int err = send_message(sock, id, Type::WRITE_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int create_request(int sock, int id, CreateRequest *req) {
    CreateResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        int fd = creat(path.c_str(), req->mode());
        if (fd > 0) {
            fds[req->path()] = fd;
        } else {
            res.set_error(errno);
        }
    }

    int err = send_message(sock, id, Type::OPEN_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int mkdir_request(int sock, int id, MkdirRequest *req) {
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
    int err = send_message(sock, id, Type::MKDIR_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int unlink_request(int sock, int id, UnlinkRequest *req) {
    UnlinkResponse res;
    std::string path = std::filesystem::weakly_canonical(base_path + req->path());
    if (path.substr(0, base_path.size()) != base_path) {
        res.set_error(EACCES);
    } else {
        int err = unlink(path.c_str());
        if (err < 0) {
            res.set_error(errno);
        } else {
            res.set_error(0);
        }
    }
    int err = send_message(sock, id, Type::UNLINK_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

static int rmdir_request(int sock, int id, RmdirRequest *req) {
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
    int err = send_message(sock, id, Type::RMDIR_RESPONSE, &res);
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

static int rename_request(int sock, int id, RenameRequest *req) {
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
    int err = send_message(sock, id, Type::RENAME_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

template <typename T> int respons_handler(int sock, int id, T message) {
    (void)sock;
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
    };
}
