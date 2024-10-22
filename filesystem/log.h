#pragma once
#include <fuse3/fuse_log.h>

void fuse_log_wrapper(enum fuse_log_level level, const char *fmt, va_list args);
