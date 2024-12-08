#pragma once
#include "../common/handlers.h"
#include "../common/tcp.h"
#include <string>
#include <unistd.h>

int listen(int port, recv_handlers fs, recv_handlers event);

int close();
