#include <string>
#include "../common/log.h"

std::string banner = R"(
 _                __
| |_ ___  __ _   / _|___
| __/ _ \/ _` | | |_/ __|
| ||  __/ (_| | |  _\__ \
 \__\___|\__,_| |_| |___/
)";

int main() {
  log(NONE, banner.c_str());
  return 0;
};
