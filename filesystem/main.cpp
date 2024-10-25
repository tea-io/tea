#include "../common/log.h"
#include "fs.h"
#include "log.h"
#include "tcp.h"
#include <fuse3/fuse.h>
#include <fuse3/fuse_log.h>
#include <string>
#include <thread>
#include <unistd.h>

std::string banner = R"(
 _                __
| |_ ___  __ _   / _|___
| __/ _ \/ _` | | |_/ __|
| ||  __/ (_| | |  _\__ \
 \__\___|\__,_| |_| |___/
)";

static struct options {
    const char *host;
    int port;
    bool debug;
    bool show_help;
    const char *name;
} opts;

#define OPTION(t, p) {t, offsetof(struct options, p), 1}
static const struct fuse_opt option_spec[] = {
    OPTION("--host=%s", host), OPTION("-h=%s", host),       OPTION("--port=%d", port), OPTION("-p=%d", port), OPTION("-d", debug),
    OPTION("--debug", debug),  OPTION("--help", show_help), OPTION("--name=%s", name), OPTION("-n=%s", name), FUSE_OPT_END};

static void show_help(char *progname) {
    log(NONE, "usage: %s [options] <mountpoint>\n\n", progname);
    log(NONE, "File-system specific options:\n"
              "    -h   --host=<s>      The host of server (required)\n"
              "    -p   --port=<d>      The port of server (default: 5210)\n"
              "    -d,  --debug         Enable debug output\n"
              "    -n   --name=<s>      The display name of user (default: login name)\n"
              "    --help               Print this help\n");
}

int main(int argc, char *argv[]) {
    set_debug_log(true);
    log(NONE, banner.c_str());

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    char *login = getlogin();
    opts.port = 5210;
    opts.name = strdup(login);
    opts.host = NULL;

    if (fuse_opt_parse(&args, &opts, option_spec, NULL) == -1) {
        return 1;
    }
    int sock = -1;
    std::thread t;
    if (opts.show_help) {
        show_help(args.argv[0]);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    } else {
        fuse_set_log_func(fuse_log_wrapper);
        if (opts.host == NULL && opts.show_help == false) {
            log(ERROR, "Host is required");
            return 1;
        }

        sock = connect(opts.host, opts.port);
        if (sock < 0) {
            return 1;
        }

        t = std::thread(recv_thread, sock);
    }

    config cfg = {.name = opts.name};

    struct fuse_operations oper = get_fuse_operations(sock, cfg);

    int ret = fuse_main(args.argc, args.argv, &oper, NULL);
    fuse_opt_free_args(&args);

    if (!opts.show_help) {
        t.detach();
        close(sock);
    }
    return ret;
};
