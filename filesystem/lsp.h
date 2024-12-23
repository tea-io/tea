#pragma once

#include "../proto/messages.pb.h"
#include <openssl/crypto.h>

int lsp_request_handler(int socket, SSL *ssl, int id, int language_id, char *request);
int lsp_response_handler(int sock, SSL *ssl, int id, LspResponse *response);
void set_lsp_extension_socket(int sock);

static std::vector<std::pair<std::string, int>> language_ids = {{"c", 10}, {"cpp", 11}, {"latex", 300}};

struct LspHeader {
    int32_t size;
    int32_t id;
    int32_t language_id;
};
