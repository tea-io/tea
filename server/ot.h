#pragma once

#include "../proto/messages.pb.h"

int transform(WriteOperation *req, WriteOperation *bef);

int ot(std::list<WriteOperation *> reqs, std::string path);

int ot_add(WriteOperation *req, std::string path);
