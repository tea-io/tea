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
    int extension_port;
    bool show_help;
    const char *name;
    bool vim_mode;
} opts;

#define OPTION(t, p) {t, offsetof(struct options, p), 1}
static const struct fuse_opt option_spec[] = {OPTION("--host=%s", host),
                                              OPTION("-h=%s", host),
                                              OPTION("--port=%d", port),
                                              OPTION("-p=%d", port),
                                              OPTION("--extension-port=%d", extension_port),
                                              OPTION("-e=%d", extension_port),
                                              OPTION("--help", show_help),
                                              OPTION("--name=%s", name),
                                              OPTION("-n=%s", name),
                                              OPTION("--vim", vim_mode),
                                              OPTION("-v", vim_mode),
                                              FUSE_OPT_END};

static void show_help(char *progname) {
    log(NONE, "usage: %s [options] <mountpoint>\n\n", progname);
    log(NONE, "File-system specific options:\n"
              "    -h   --host=<s>                The host of server (required)\n"
              "    -p   --port=<d>                The port of server (default: 5210)\n"
              "    -e   --extension-port=<d>      The port of server (default: 5211)\n"
              "    -n   --name=<s>                The display name of user (default: login name)\n"
              "    -v   --vim                     Enable vim mode\n"
              "    --help                         Print this help\n");
}

static void cleanup_routine(fuse_args *args, const int sock_fd) {
    fuse_opt_free_args(args);
    if (sock_fd >= 0)
        close(sock_fd);
};

int main(int argc, char *argv[]) {
    set_debug_log(false);
    log(NONE, banner.c_str());

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    char *login = getlogin();
    if (login == NULL) {
        std::string unknown = "unknown";
        login = strdup(unknown.c_str());
    }
    opts.port = 5210;
    opts.extension_port = 5211;
    opts.name = strdup(login);
    opts.host = NULL;
    opts.vim_mode = false;

    if (fuse_opt_parse(&args, &opts, option_spec, NULL) == -1) {
        cleanup_routine(&args, -1);
        return 1;
    }
    int sock = -1;
    int ext_sock = -1;
    if (opts.show_help) {
        show_help(args.argv[0]);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    } else {
        fuse_set_log_func(fuse_log_wrapper);
        if (opts.host == NULL && opts.show_help == false) {
            log(ERROR, "Host is required");
            cleanup_routine(&args, sock);
            return 1;
        }

        sock = connect(opts.host, opts.port);
        if (sock < 0) {
            cleanup_routine(&args, sock);
            return 1;
        }

        ext_sock = listen(opts.extension_port);
        log(INFO, ext_sock, "Listen on port %d", opts.extension_port);
        if (ext_sock < 0) {
            cleanup_routine(&args, sock);
            return 1;
        }
    }

    config cfg = {.name = opts.name, .vim_mode = opts.vim_mode};

    struct fuse_operations oper = get_fuse_operations(sock, ext_sock, cfg);

    int ret = fuse_main(args.argc, args.argv, &oper, NULL);

    cleanup_routine(&args, sock);
    return ret;
};
