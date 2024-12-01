#include "local_copy.h"
#include "../common/log.h"
#include <algorithm>
#include <map>
#include <string>

std::map<int, std::string> copies_store;

void init_local_copy(int fd) { copies_store[fd] = ""; }

void discard_local_copy(int fd) {
    if (const auto copy = copies_store.find(fd); copy != copies_store.end()) {
        copies_store.erase(copy);
        log(DEBUG, "Discarded local copy for fd %d", fd);
    } else {
        log(DEBUG, "Local copy for fd %d not found. Ignoring...", fd);
    }
}

void patch_local_copy(int fd, const char *data, size_t size, size_t offset) {
    const auto copy = copies_store.find(fd);
    if (copy == copies_store.end()) {
        // This should never happen, but just in case we will write it to the local copy
        log(DEBUG, "Local copy for fd %d not found. Creating a new one...", fd);
        init_local_copy(fd);
    }

    auto &local_copy = copy->second;
    local_copy.resize(std::max(local_copy.length(), offset + size - 1));

    std::copy_n(data, size, local_copy.begin() + offset);
}

void truncate_local_copy(int fd, size_t size) {
    if (auto copy = copies_store.find(fd); copy != copies_store.end()) {
        copy->second.resize(size);
    } else {
        log(DEBUG, "Local copy for fd %lu not found.", fd);
    }
}

std::string get_local_copy(int fd) {
    if (const auto copy = copies_store.find(fd); copy != copies_store.end()) {
        return copy->second;
    }
    return "";
}

std::string get_local_copy(int fd, size_t size, size_t offset) {
    std::string copy = get_local_copy(fd);
    if (offset >= copy.size()) {
        return "";
    }
    if (offset + size > copy.size()) {
        return copy.substr(offset);
    }
    return copy.substr(offset, size);
}
