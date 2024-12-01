#include "local_copy.h"

#include "../common/log.h"
#include "../proto/messages.pb.h"
#include "tcp.h"

#include <map>
#include <string>

std::map<std::string, std::string> copies_store;

void ensure_local_copy_initialized(const int socket, const char *path) {
    auto attrReq = GetAttrRequest();
    attrReq.set_path(path);

    auto attrResp = GetAttrResponse();
    auto err = request_response<GetAttrResponse>(socket, attrReq, &attrResp, GET_ATTR_REQUEST);
    if (err < 0) {
        log(ERROR, socket, "Error sending message");
        return;
    }
    log(INFO, socket, "Received attributes, file size is: %d", attrResp.size());

    auto contentReq = ReadRequest();
    contentReq.set_path(path);
    contentReq.set_offset(0);
    contentReq.set_size(attrResp.size());

    auto contentResp = ReadResponse();
    err = request_response<ReadResponse>(socket, contentReq, &contentResp, READ_REQUEST);
    if (err < 0) {
        log(ERROR, socket, "Error sending message");
        return;
    }

    copies_store[path] = contentResp.data();
}

void discard_local_copy(const char *path) {
    if (const auto copy = copies_store.find(path); copy != copies_store.end()) {
        copies_store.erase(copy);
        log(INFO, "Discarded local copy for fd %s", path);
    } else {
        log(ERROR, "Local copy for fd %lu not found. Ignoring...", path);
    }
}

void patch_local_copy(const char *path, const char *data, size_t size, size_t offset) {
    const auto copy = copies_store.find(path);
    if (copy == copies_store.end()) {
        // This should never happen, but just in case we will write it to the local copy
        log(ERROR, "Local copy for fd %lu not found.", path);
        copies_store[path] = std::string(data, size);
        return;
    }

    auto &local_copy = copy->second;
    if (offset > local_copy.size()) {
        log(ERROR, "Offset %lu is greater than the size of the local copy %lu", offset, local_copy.size());
        return;
    }
    local_copy.resize(std::max(local_copy.length(), offset + size));

    std::copy_n(data, size, local_copy.begin() + offset);
}

void truncate_local_copy(const char *path, size_t size) {
    if (auto copy = copies_store.find(path); copy != copies_store.end()) {
        copy->second.resize(size);
    } else {
        log(ERROR, "Local copy for fd %lu not found.", path);
    }
}

std::optional<std::string_view> get_local_copy(const char *path) {
    if (const auto copy = copies_store.find(path); copy != copies_store.end()) {
        return copy->second;
    }
    return std::nullopt;
}
