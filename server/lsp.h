#pragma once

#include <map>
#include <string>

#include "../proto/messages.pb.h"

class LspProcess {
  public:
    explicit LspProcess(const std::string &language);
    ~LspProcess();
    void write(const std::string &data) const;
    [[nodiscard]] std::optional<std::string> read() const;

  private:
    pid_t pid;
    int write_fd;
    int read_fd;
};

static std::map<std::string, std::string> available_lsps = {};

static std::map<std::string, std::shared_ptr<LspProcess>> lsp_handles = {};

void start_lsp_servers(const std::string &server_base_path);
int handle_lsp_request(int sock, int id, LspRequest *request);
void initialize_lsp_config(std::string server_base_path);