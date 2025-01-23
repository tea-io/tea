#include "../common/log.h"
#include "fs.h"
#include "log.h"
#include "tcp.h"
#include <fuse3/fuse.h>
#include <fuse3/fuse_log.h>
#include <gnutls/gnutls.h>
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
    bool show_help;
    const char *name;
    const char *cert;
    const char *key;
    const char *srvcert;
} opts;

#define OPTION(t, p) {t, offsetof(struct options, p), 1}
static const struct fuse_opt option_spec[] = {
    OPTION("--host=%s", host), OPTION("-h=%s", host),          OPTION("--port=%d", port), OPTION("-p=%d", port), OPTION("--help", show_help),
    OPTION("--name=%s", name), OPTION("-n=%s", name),          OPTION("--cert=%s", cert), OPTION("-c=%s", cert), OPTION("--key=%s", key),
    OPTION("-k=%s", key),      OPTION("--server=%s", srvcert), OPTION("-s=%s", srvcert),  FUSE_OPT_END};

static void show_help(char *progname) {
    log(NONE, "usage: %s [options] <mountpoint>\n\n", progname);
    log(NONE, "File-system specific options:\n"
              "    -h   --host=<s>      The host of server (required)\n"
              "    -p   --port=<d>      The port of server (default: 5210)\n"
              "    -n   --name=<s>      The display name of user (default: login name)\n"
              "    -c   --cert=<s>      Client x509 PEM certificate path (default: '')\n"
              "    -k   --key=<s>       Client x509 PEM certificate key path (default: '')\n"
              "    -s   --server=<s>    Server x509 PEM certificate path (optional) (default: '')\n"
              "    --help               Print this help\n");
}

static void cleanup_routine(fuse_args *args, const int sock_fd, gnutls_session_t *session) {
    fuse_opt_free_args(args);
    if (session != nullptr) {
        gnutls_bye(*session, GNUTLS_SHUT_RDWR);
        gnutls_deinit(*session);
    }
    if (sock_fd >= 0)
        close(sock_fd);
};

int main(int argc, char *argv[]) {
    log(NONE, banner.c_str());

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    char *login = getlogin();
    if (login == NULL) {
        std::string unknown = "unknown";
        login = strdup(unknown.c_str());
    }
    opts.port = 5210;
    opts.name = strdup(login);
    opts.host = NULL;
    opts.cert = NULL;
    opts.key = NULL;
    opts.srvcert = NULL;

    if (fuse_opt_parse(&args, &opts, option_spec, NULL) == -1) {
        cleanup_routine(&args, -1, nullptr);
        return 1;
    }

    gnutls_session_t session;
    gnutls_certificate_credentials_t cred;
    int sock = -1;
    if (opts.show_help) {
        show_help(args.argv[0]);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    } else {
        fuse_set_log_func(fuse_log_wrapper);
        if (opts.host == NULL && opts.show_help == false) {
            log(ERROR, "Host is required");
            cleanup_routine(&args, sock, nullptr);
            return 1;
        }
        if (opts.cert == NULL || opts.key == NULL) {
            log(ERROR, "Missing TLS key/certificate");
            cleanup_routine(&args, -1, nullptr);
        }

        gnutls_global_init();

        gnutls_certificate_allocate_credentials(&cred);
        int err = gnutls_certificate_set_x509_key_file(cred, opts.cert, opts.key, GNUTLS_X509_FMT_PEM);
        if (err < 0) {
            log(ERROR, "Failed to set certificate/key: %s", gnutls_strerror(err));
            cleanup_routine(&args, -1, nullptr);
            return 1;
        }

        if (opts.srvcert != NULL) {
            gnutls_certificate_set_x509_trust_file(cred, opts.srvcert, GNUTLS_X509_FMT_PEM);
        }

        gnutls_init(&session, GNUTLS_CLIENT);
        gnutls_set_default_priority(session);
        gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, cred);

        sock = connect(opts.host, opts.port);
        if (sock < 0) {
            cleanup_routine(&args, sock, &session);
            return 1;
        }

        gnutls_transport_set_int(session, sock);

        if (gnutls_handshake(session) < 0) {
            log(ERROR, "TLS handshake failed");
            cleanup_routine(&args, sock, &session);
            return 1;
        }
    }

    std::thread lsp_thread(listen_lsp, 5211, sock, session);

    config cfg = {.name = opts.name};

    struct fuse_operations oper = get_fuse_operations(sock, cfg, session);

    int ret = fuse_main(args.argc, args.argv, &oper, NULL);

    cleanup_routine(&args, sock, &session);
    if (cred != nullptr) {
        gnutls_certificate_free_credentials(cred);
    }
    gnutls_global_deinit();
    return ret;
};
