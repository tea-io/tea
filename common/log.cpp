#include "log.h"
#include <cstdarg>
#include <cstdio>
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define NORMAL "\033[0m"

bool debug_log = true;

void raw_log(int level, const char *content) {
    switch (level) {
    case INFO:
        fprintf(stdout, "%s[INFO]%s %s\n", GREEN, NORMAL, content);
        break;
    case WARN:
        fprintf(stderr, "%s[WARN]%s %s\n", YELLOW, NORMAL, content);
        break;
    case ERROR:
        fprintf(stderr, "%s[ERROR]%s %s\n", RED, NORMAL, content);
        break;
    case DEBUG:
        if (debug_log)
            fprintf(stderr, "%s[DEBUG]%s %s\n", BLUE, NORMAL, content);
        break;
    default:
        fprintf(stdout, "%s\n", content);
    }
}

void log(int level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    raw_log(level, buffer);
}

void log(int level, int socket, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    log(level, "[%d] %s", socket, buffer);
}

void set_debug_log(bool enable) { debug_log = enable; }
