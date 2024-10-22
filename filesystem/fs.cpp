#include "fs.h"
#include "../common/log.h"
#include "../proto/messages.pb.h"
#include "tcp.h"

int sock;

static void *init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    (void)conn;
    (void)cfg;
    InitRequest req = InitRequest();
    req.set_error(0);
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
fuse_operations get_fuse_operations(int sockFd) {
    sock = sockFd;
    fuse_operations ops = {
        .init = init,
    };
    return ops;
};
#pragma GCC diagnostic pop
