#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "lsp.h"

#include "../common/io.h"
#include "../common/log.h"
#include "../proto/messages.pb.h"
#include "google/protobuf/util/json_util.h"

#include <fstream>
#include <iomanip>
#include <regex>

static std::regex method_regex(R"("method":\s?"initialize")", std::regex_constants::ECMAScript | std::regex_constants::icase);
static std::regex base_path_regex(R"(rootPath":\s*"([^"]+))", std::regex_constants::ECMAScript | std::regex_constants::icase);

static std::optional<std::string> base_path = std::nullopt;
static std::string server_path = {};

LspProcess::LspProcess(const std::string &language) {
    if (available_lsps.find(language) == available_lsps.end()) {
        throw std::runtime_error("Language not supported");
    }

    int parent_to_child[2];
    int child_to_parent[2];

    if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1) {
        throw std::runtime_error("Failed to create pipe");
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        throw std::runtime_error("Failed to fork");
    }

    if (pid == 0) {
        close(parent_to_child[1]);
        close(child_to_parent[0]);

        dup2(parent_to_child[0], STDIN_FILENO);
        dup2(child_to_parent[1], STDOUT_FILENO);

        close(parent_to_child[0]);
        close(child_to_parent[1]);

        execl("/bin/sh", "sh", "-c", available_lsps[language].c_str(), nullptr);
        exit(1);
    }

    close(parent_to_child[0]);
    close(child_to_parent[1]);

    write_fd = parent_to_child[1];
    read_fd = child_to_parent[0];
    fcntl(read_fd, F_SETFL, fcntl(read_fd, F_GETFL) | O_NONBLOCK);
}

LspProcess::~LspProcess() {
    close(write_fd);
    close(read_fd);
    kill(pid, SIGTERM);
    waitpid(pid, nullptr, 0);
}

static bool is_initialization(const std::string &data) { return std::regex_search(data, method_regex); }

static std::optional<std::string> extract_base_path(const std::string &data) {
    if (!is_initialization(data)) {
        return std::nullopt;
    }

    if (base_path.has_value()) {
        log(WARN, "Duplicate initialization message received");
    }

    if (std::smatch match; std::regex_search(data, match, base_path_regex)) {
        auto extracted_base_path = match[1].str();
        log(DEBUG, "Base path is %s", extracted_base_path.c_str());
        log(INFO, "Paths will be translated %s <=> %s", server_path.c_str(), extracted_base_path.c_str());

        return extracted_base_path;
    }

    log(WARN, "Message type was initialization but no base path was found");
    return std::nullopt;
}

static std::string urlEncode(const std::string &str) {
    std::ostringstream encodedStream;
    encodedStream << std::hex << std::uppercase << std::setfill('0');

    for (const char c : str) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/') {
            encodedStream << c;
        } else {
            encodedStream << '%' << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(c));
        }
    }

    return encodedStream.str();
}

static std::string patch_paths(const std::string &data, const std::string &replace_this, const std::string &with_this) {
    auto patched_data = data;

    std::string::size_type start_pos = 0;
    while ((start_pos = patched_data.find(replace_this, start_pos)) != std::string::npos) {
        patched_data.replace(start_pos, replace_this.length(), with_this);
        start_pos += with_this.length() + 1;
    }

    return patched_data;
}

void LspProcess::write(const std::string &data) const {
    assert(base_path.has_value());

    const auto url_base_path = urlEncode(::base_path.value());
    const auto url_server_path = urlEncode(::server_path);
    auto patched_data = patch_paths(data, ::base_path.value(), ::server_path);
    patched_data = patch_paths(patched_data, url_base_path, url_server_path);

    char buffer[50];
    std::sprintf(buffer, "Content-Length: %zu\r\n\r\n", patched_data.size());
    ::write(write_fd, buffer, std::strlen(buffer));

    const auto n = ::write(write_fd, patched_data.c_str(), patched_data.size());
    log(DEBUG, "Wrote %d bytes to LSP", n);
}

std::optional<std::string> LspProcess::read() const {
    std::array<char, 1024> buffer{};
    std::string result;

    pollfd pfd = {.fd = read_fd, .events = POLLIN, .revents = 0};

    while (poll(&pfd, 1, 100) > 0) {
        if (pfd.revents & POLLIN) {
            if (const ssize_t n = ::read(read_fd, buffer.data(), buffer.size()); n > 0) {
                result.append(buffer.data(), n);
            }
        }
    }

    const auto end_of_headers = result.find("\r\n\r\n");
    if (end_of_headers == std::string::npos) {
        return std::nullopt;
    }

    assert(::base_path.has_value());
    const auto json_rpc = result.substr(end_of_headers + 4);
    const auto url_base_path = urlEncode(::base_path.value());
    const auto url_server_path = urlEncode(::server_path);
    auto patched_data = patch_paths(json_rpc, ::base_path.value(), ::server_path);
    patched_data = patch_paths(patched_data, url_base_path, url_server_path);

    return patched_data;
}

static int start_server(const std::string &language_name) {
    if (const auto command = available_lsps.find(language_name); command == available_lsps.end()) {
        log(ERROR, "Language not supported");
        return -1;
    }

    lsp_handles[language_name] = std::make_shared<LspProcess>(language_name);
    return 0;
}

int handle_lsp_request(int sock, gnutls_session_t ssl, int id, LspRequest *request) {
    if (is_initialization(request->payload())) {
        if (!::base_path.has_value()) {
            ::base_path = extract_base_path(request->payload());
        }

        start_server(request->language());
    }

    const auto &language = request->language();
    auto language_handler = lsp_handles.find(language);
    if (language_handler == lsp_handles.end()) {
        if (start_server(language) < 0) {
            return -1;
        }

        language_handler = lsp_handles.find(language);
    }
    const auto handler = language_handler->second;

    const auto &response = request->payload();
    handler->write(response);

    const auto data = handler->read();
    if (!data.has_value()) {
        return 0;
    }

    LspResponse res;
    res.set_payload(data.value());
    res.set_language(language);

    const auto n = send_message(sock, ssl, id, Type::LSP_RESPONSE, &res);
    return n < 0 ? -1 : n;
}

void initialize_lsp_config(std::string server_base_path) {
    ::server_path = std::move(server_base_path);

    auto config_home = getenv("XDG_CONFIG_HOME");
    if (config_home == nullptr) {
        const auto passwd = getpwuid(getuid());
        const auto home = passwd->pw_dir;

        if (home == nullptr) {
            log(ERROR, "HOME is not set. No LSPs will be available.");
            return;
        }
        config_home = static_cast<char *>(std::malloc(strlen(home) + strlen("/.config") + 1));
        strcpy(config_home, home);
        strcat(config_home, "/.config");
    }

    auto config_file = std::ifstream(std::string(config_home) + "/tea/config.json");
    if (!config_file.is_open()) {
        log(ERROR, "Unable to open config file. No LSPs will be available. Error is: %s", strerror(errno));
        log(ERROR, "Please create a config file at $XDG_CONFIG_HOME/tea/config.json");
        return;
    }
    const auto json_string = std::string(std::istreambuf_iterator(config_file), std::istreambuf_iterator<char>());

    auto config = TeaConfigFile{};
    if (const auto status = google::protobuf::json::JsonStringToMessage(json_string, &config); !status.ok()) {
        log(ERROR, "Failed to deserialize config file: %s", status.ToString());
        return;
    }

    ::available_lsps.clear();
    for (const auto &[language, server] : config.language_configs()) {
        ::available_lsps[language] = server;
        log(DEBUG, "LSP for %s is %s", language.c_str(), server.c_str());
    }
}

void reset_handlers() { ::lsp_handles.clear(); }
