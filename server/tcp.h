#pragma once
#include "../common/io.h"
#include <string>

int listen(int port, recv_handlers fs, recv_handlers event);

int close_connections();
