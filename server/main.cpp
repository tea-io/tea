#include "../common/hash.h"
#include "../common/log.h"
#include "fs.h"
#include "tcp.h"
#include <filesystem>
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
    if (argc < 2) {
        log(ERROR, "Usage: %s <dir>", argv[0]);
        return 1;
    }
    std::filesystem::path path = argv[1];
    if (!std::filesystem::exists(path)) {
        log(ERROR, "Directory %s does not exist", argv[1]);
        return 1;
    }
    set_debug_log(true);
    log(NONE, banner.c_str());
    init_crc_table();
    listen(5210, get_handlers(path));
    return 0;
};
