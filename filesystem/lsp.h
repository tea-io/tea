#pragma once

#include "../proto/messages.pb.h"

int lsp_request_handler(int socket, int id, char *request);
int lsp_response_handler(int sock, int id, LspResponse *response);
void set_lsp_extension_socket(int sock);