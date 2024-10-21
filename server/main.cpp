#include "../common/log.h"
#include "fs.h"
#include "tcp.h"
#include <string>

std::string banner = R"(
 _
| |_ ___  __ _   ___  ___ _ ____   _____ _ __
| __/ _ \/ _` | / __|/ _ \ '__\ \ / / _ \ '__|
| ||  __/ (_| | \__ \  __/ |   \ V /  __/ |
 \__\___|\__,_| |___/\___|_|    \_/ \___|_|
)";

int main() {
    set_debug_log(true);
    log(NONE, banner.c_str());
    listen(5210, get_handlers());
    return 0;
};
