#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../proto/messages.pb.h"

std::vector<WriteOperation> diff(const std::string &e, const std::string &f);

std::vector<WriteOperation> diff(const std::string &e, const std::string &f, size_t offset);
