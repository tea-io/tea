#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../proto/messages.pb.h"

std::vector<WriteRequest> diff(const std::string &e, const std::string &f);

void enable_diff(std::string file);
void disable_diff(std::string file);
bool is_diff_enabled(std::string file);

int diff_enable_handler(int sock, int id, DiffWriteEnable *event);
int diff_disable_handler(int sock, int id, DiffWriteDisable *event);
