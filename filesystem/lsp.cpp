#include "lsp.h"
#include "../common/log.h"

#include "../common/header.h"
#include "../common/io.h"

static int lsp_client_sock = -1;

void set_lsp_extension_socket(const int sock) {
    if (lsp_client_sock > 0) {
        log(ERROR, "LSP client socket already initialized");
    }

    lsp_client_sock = sock;
}

int lsp_request_handler(const int sock, const int id, char *request) {
    LspRequest req;
    req.set_payload(request);

    const auto n = send_message(sock, id, Type::LSP_REQUEST, &req);
    return n < 0 ? -1 : n;
}

int lsp_response_handler(int sock, int id, LspResponse *response) {
    if (lsp_client_sock < 0) {
        log(ERROR, sock, "LSP client socket not initialized");
        return -1;
    }

    const auto payload = response->payload();
    auto header = Header{
        .size = static_cast<int32_t>(payload.size()),
        .id = ++id,
        .type = 0,
    };
    const auto serialized = serialize(&header);

    const auto buffer_size = HEADER_SIZE + payload.size();
    const auto write_buffer = new char[buffer_size];
    std::memcpy(write_buffer, serialized, HEADER_SIZE);
    std::memcpy(write_buffer + HEADER_SIZE, payload.c_str(), payload.size());

    auto n = 0;
    if (n = write(lsp_client_sock, write_buffer, buffer_size); n < 0) {
        log(ERROR, lsp_client_sock, "Failed to write to lsp extension socket, error: %s", std::strerror(errno));
        return -1;
    }

    delete[] serialized;
    delete[] write_buffer;
    return n;
}
