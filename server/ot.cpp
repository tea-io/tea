#include "ot.h"
#include <algorithm>

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

// ignore wswitch-enum
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
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
#pragma GCC diagnostic pop
