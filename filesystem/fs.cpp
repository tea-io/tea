#include "fs.h"
#include "../common/log.h"
#include "../proto/messages.pb.h"
#include "tcp.h"

int sock;
config cfg;

static void *init(struct fuse_conn_info *conn, struct fuse_config *f_cfg) {
    (void)conn;
    (void)f_cfg;
    InitRequest req = InitRequest();
    req.set_name(cfg.name);
    InitResponse res;
    int err = request_response<InitResponse>(sock, req, &res);
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
fuse_operations get_fuse_operations(int sock_fd, config cfg_param) {
    sock = sock_fd;
    cfg = cfg_param;
    fuse_operations ops = {
        .init = init,
        .destroy = destroy,
    };
    return ops;
};
#pragma GCC diagnostic pop
