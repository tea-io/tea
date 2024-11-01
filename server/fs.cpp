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
    };
}
