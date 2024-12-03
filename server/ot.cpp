#include "ot.h"
#include "../common/hash.h"
#include <algorithm>
#include <fcntl.h>

std::vector<WriteOperation> operations;

// transformation all kind of operations based on the previous append operation
static int append_all(WriteOperation *bef, WriteOperation *req) {
    if (req->offset() >= bef->offset()) {
        req->set_offset(req->offset() + bef->size());
    }
    return 0;
}

// transformation a delete operation based on the previous append operation
static int delete_append(WriteOperation *bef, WriteOperation *req) {
    if (req->offset() >= bef->offset()) {
        if (req->offset() < bef->offset() + bef->size()) {
            req->set_offset(bef->offset());
        } else {
            req->set_offset(req->offset() - bef->size());
        }
    }
    return 0;
}

// transformation a delete operation based on the previous delete operation
static int delete_delete(WriteOperation *bef, WriteOperation *req) {
    int start = std::max(req->offset(), bef->offset());
    int end = std::min(req->offset() + req->size(), bef->offset() + bef->size());
    int size = end - start;
    if (size > 0) {
        req->set_size(req->size() - size);
        if (req->offset() >= bef->offset()) {
            req->set_offset(bef->offset());
        }
        return 0;
    }
    if (req->offset() >= bef->offset()) {
        req->set_offset(req->offset() - bef->size());
    }
    return 0;
}

// transformation a delete operation based on the previous replace operation
static int delete_replace(WriteOperation *bef, WriteOperation *req) {
    int start = std::max(req->offset(), bef->offset());
    int end = std::min(req->offset() + req->size(), bef->offset() + bef->size());
    int size = end - start;
    if (size > 0) {
        req->set_size(0);
        req->set_offset(0);
        req->set_flag(DELETE);
        req->set_data("");
    }
    if (req->offset() >= bef->offset()) {
        req->set_offset(req->offset() - bef->size());
    }
    return 0;
}

int transform(WriteOperation *req, WriteOperation *bef) {
    switch (bef->flag()) {
    case REPLACE:
        return 0;
    case APPEND:
        return append_all(bef, req);
    case DELETE:
        switch (req->flag()) {
        case REPLACE:
            return delete_replace(bef, req);
        case APPEND:
            return delete_append(bef, req);
        case DELETE:
            return delete_delete(bef, req);
        default:
            return 0;
        }
    default:
        return 0;
    }
}

static uint32_t hash_current(std::string path) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    char buf[4096];
    crc32_begin();
    while (true) {
        int err = read(fd, buf, 4096);
        if (err < 0) {
            close(fd);
            return -1;
        }
        if (err == 0) {
            break;
        }
        crc32_sum(buf, err);
    }
    return crc32_end();
}

int ot(std::list<WriteOperation *> reqs, std::string path) {
    WriteOperation *req = reqs.front();
    uint32_t hash = hash_current(path);
    if (req->hash() != hash) {
        return 0;
    }

    auto coll = std::find_if(operations.begin(), operations.end(), [req](WriteOperation op) { return op.hash() == req->hash(); });

    if (coll == operations.end()) {
        return -1;
    }

    coll++;

    for (auto it = coll; it != operations.end(); it++) {
        for (auto it_req = reqs.begin(); it_req != reqs.end(); it_req++) {
            transform(*it_req, &(*it));
        }
    }
    return 0;
}

int ot_add(WriteOperation *req, std::string path) {
    uint32_t hash = hash_current(path);
    if (hash == -1) {
        return -2;
    }
    req->set_hash(hash);
    operations.insert(operations.begin(), *req);
    if (operations.size() > 20) {
        operations.pop_back();
    }
    return 0;
};
