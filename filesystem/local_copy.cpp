#include "local_copy.h"
#include "../common/log.h"
#include <algorithm>
#include <map>
#include <string>

std::map<std::string, std::string> copies_store;

void patch_copy(std::string path, const char *data, size_t size, size_t offset) {
    auto copy = copies_store.find(path);
    if (copy == copies_store.end()) {
        log(DEBUG, "Creating a new copy for %s ...", path.c_str());
        copies_store[path] = "";
        copy = copies_store.find(path);
    }

    auto &local_copy = copy->second;
    local_copy.resize(offset + size - 1);

    std::copy_n(data, size, local_copy.begin() + offset);
}

void truncate_copy(std::string path, size_t size) {
    if (auto copy = copies_store.find(path); copy != copies_store.end()) {
        copy->second.resize(size);
    } else {
        copies_store[path] = "";
        copies_store[path].resize(size);
        log(DEBUG, "Creating a new copy for %s ...", path.c_str());
    }
}

std::string get_copy(std::string path) {
    if (const auto copy = copies_store.find(path); copy != copies_store.end()) {
        return copy->second;
    }
    return "";
}

std::string get_copy(std::string path, size_t size, size_t offset) {
    std::string copy = get_copy(path);
    if (offset >= copy.size()) {
        return "";
    }
    if (offset + size > copy.size()) {
        return copy.substr(offset);
    }
    return copy.substr(offset, size);
}

void discard_copy(std::string path) { copies_store.erase(path); }
