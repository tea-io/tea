#include "../common/log.h"
#include "../server/lsp.h"
#include "fs.h"
#include "tcp.h"
#include <filesystem>
#include <getopt.h>
#include <string>
#include <sys/stat.h>

std::string banner = R"(
 _
| |_ ___  __ _   ___  ___ _ ____   _____ _ __
| __/ _ \/ _` | / __|/ _ \ '__\ \ / / _ \ '__|
| ||  __/ (_| | \__ \  __/ |   \ V /  __/ |
 \__\___|\__,_| |___/\___|_|    \_/ \___|_|
)";

int main(int argc, char *argv[]) {
    if (argc < 4) {
        log(ERROR, "Usage: %s <dir> <cert> <key>", argv[0]);
        return 1;
    }
    std::filesystem::path path = argv[1];
    std::string cert = argv[2];
    std::string key = argv[3];

    if (!std::filesystem::exists(path)) {
        log(ERROR, "Directory %s does not exist", argv[1]);
        return 1;
    }
    set_debug_log(true);
    log(NONE, banner.c_str());

    start_lsp_servers(path);
    listen(5210, get_handlers(path), cert, key);
    return 0;
};
