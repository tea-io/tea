#include "log.h"
#include "../common/log.h"
#include <cstdio>

int fuse_levels_to_internal[] = {WARN, WARN, ERROR, ERROR, WARN, INFO, INFO, DEBUG};

void fuse_log_wrapper(enum fuse_log_level level, const char *fmt, va_list args) {
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    raw_log(level, buffer);
};
