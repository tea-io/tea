#include "log.h"
#include "../common/log.h"
#include <cstdio>

// enum fuse_log_level { FUSE_LOG_EMERG, FUSE_LOG_ALERT, FUSE_LOG_CRIT, FUSE_LOG_ERR, FUSE_LOG_WARNING, FUSE_LOG_NOTICE, FUSE_LOG_INFO, FUSE_LOG_DEBUG };
const int fuse_levels_to_internal[] = {ERROR, ERROR, ERROR, ERROR, WARN, INFO, INFO, DEBUG};

void fuse_log_wrapper(enum fuse_log_level level, const char *fmt, va_list args) {
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    raw_log(fuse_levels_to_internal[level], buffer);
};
