#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../proto/messages.pb.h"

std::vector<WriteRequest> diff(const std::string &e, const std::string &f);
