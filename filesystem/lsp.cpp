#include "lsp.h"
#include "../common/log.h"

#include "../common/header.h"
#include "../common/io.h"

static int lsp_client_sock = -1;

static std::optional<int> find_by_language_name(const std::string_view language) {
    for (const auto &[name, id] : language_ids) {
        if (name == language) {
            return id;
        }
    }

    return std::nullopt;
}

static std::optional<std::string> find_by_language_id(const int language_id) {
    for (const auto &[name, id] : language_ids) {
        if (id == language_id) {
            return name;
        }
    }

    return std::nullopt;
}

static std::array<char, sizeof(LspHeader)> serialize_lsp(const LspHeader &header) {
    auto buf = std::array<char, sizeof(LspHeader)>();
    const auto nsize = htonl(header.size);
    const auto nid = htonl(header.id);
    const auto nlanguage_id = htonl(header.language_id);

    memcpy(buf.data(), &nsize, sizeof(int32_t));
    memcpy(buf.data() + sizeof(int32_t), &nid, sizeof(int32_t));
    memcpy(buf.data() + sizeof(int32_t) + sizeof(int32_t), &nlanguage_id, sizeof(int32_t));

    return buf;
}

void set_lsp_extension_socket(const int sock) {
    if (lsp_client_sock > 0) {
        log(INFO, "LSP client sock already initialized, overriding.");
    }

    lsp_client_sock = sock;
}

int lsp_request_handler(const int sock, SSL *ssl, const int id, const int language_id, char *request) {
    const auto language_name = find_by_language_id(language_id);
    if (!language_name.has_value()) {
        log(ERROR, sock, "Unknown language id: %d", language_id);
        return -1;
    };

    LspRequest req;
    req.set_payload(request);
    req.set_language(language_name.value());

    const auto n = send_message(sock, ssl, id, Type::LSP_REQUEST, &req);
    return n < 0 ? -1 : n;
}

int lsp_response_handler(int sock, SSL *ssl, int id, LspResponse *response) {
    if (lsp_client_sock < 0) {
        log(INFO, sock, "LSP client sock not initialized, aborting.");
        return -1;
    }

    const auto &payload = response->payload();
    const auto language_id = find_by_language_name(response->language());
    assert(language_id.has_value());

    const auto header = LspHeader{.size = static_cast<int32_t>(payload.size()), .id = ++id, .language_id = language_id.value()};
    const auto serialized_header = serialize_lsp(header);

    const auto buffer_size = HEADER_SIZE + payload.size();
    const auto write_buffer = std::make_unique<char[]>(buffer_size);

    std::memcpy(write_buffer.get(), serialized_header.data(), HEADER_SIZE);
    std::memcpy(write_buffer.get() + HEADER_SIZE, payload.c_str(), payload.size());

    auto n = 0;
    if (n = write(lsp_client_sock, write_buffer.get(), buffer_size); n < 0) {
        log(ERROR, lsp_client_sock, "Failed to write to lsp extension socket, error: %s", std::strerror(errno));
        return -1;
    }

    return n;
}
