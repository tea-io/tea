#pragma once

#include "../proto/messages.pb.h"
#include <openssl/crypto.h>

int lsp_request_handler(int socket, SSL *ssl, int id, char *request);
int lsp_response_handler(int sock, SSL *ssl, int id, LspResponse *response);
void set_lsp_extension_socket(int sock);
