#pragma once
#include "../common/io.h"
#include <string>

int listen(int port, recv_handlers hadnlers, std::string cert, std::string key);

int close_connections();
