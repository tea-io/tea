#pragma once
#include "../common/io.h"
#include <string>

int listen(int port, recv_handlers hadnlers);

int close_connections();
