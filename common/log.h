#pragma once

#include <cstdarg>

const int INFO = 0;
const int WARN = 1;
const int ERROR = 2;
const int DEBUG = 3;
const int NONE = 4;

void log(int level, const char *fmt, ...);
void log(int level, int socket, const char *fmt, ...);
void set_debug_log(bool enable);
void raw_log(int level, const char *content);
