#include "diff.h"
#include <dtl/dtl.hpp>

std::map<std::string, short> diff_mode_files;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-null-dereference"
std::vector<WriteOperation> diff(const std::string &e, const std::string &f) { return diff(e, f, 0); }

std::vector<WriteOperation> diff(const std::string &e, const std::string &f, size_t offset) {
    std::vector<WriteOperation> diffs;

    dtl::Diff<char, std::string> d(e, f);
    d.compose();

    int local_offset = -1;
    std::string agg;
    auto s = d.getSes().getSequence();
    auto type = dtl::SES_COMMON;

    for (unsigned long int i = 0; i < s.size(); i++) {
        const auto &elem = s[i];

        if (elem.second.type == dtl::SES_COMMON) {
            continue;
        }

        if (elem.second.type != type || local_offset == -1) {
            if (!agg.empty()) {
                WriteOperation wr = WriteOperation();
                wr.set_flag(type == dtl::SES_ADD ? APPEND : DELETE);
                wr.set_data(agg);
                wr.set_offset(local_offset + offset);
                wr.set_size(agg.size());
                diffs.push_back(wr);
            }

            if (elem.second.type == dtl::SES_ADD) {
                local_offset = elem.second.afterIdx - 1;
            } else {
                local_offset = elem.second.beforeIdx - 1;
            }
            agg.clear();
            type = elem.second.type;
        }
        agg += elem.first;
    }

    if (!agg.empty()) {
        WriteOperation wr = WriteOperation();
        wr.set_flag(type == dtl::SES_ADD ? APPEND : DELETE);
        wr.set_data(agg);
        wr.set_offset(local_offset + offset);
        wr.set_size(agg.size());
        diffs.push_back(wr);
    }

    return diffs;
}
#pragma GCC diagnostic pop

int diff_enable_handler(int sock, int id, DiffWriteEnable *event) {
    (void)sock;
    (void)id;
    enable_diff(event->path());
    return 0;
}

int diff_disable_handler(int sock, int id, DiffWriteDisable *event) {
    (void)sock;
    (void)id;
    disable_diff(event->path());
    return 0;
}

void enable_diff(std::string file) {
    if (diff_mode_files.find(file) == diff_mode_files.end()) {
        diff_mode_files[file] = 1;
    } else {
        diff_mode_files[file]++;
    }
}

void disable_diff(std::string file) {
    if (diff_mode_files.find(file) != diff_mode_files.end()) {
        diff_mode_files[file]--;
        if (diff_mode_files[file] == 0) {
            diff_mode_files.erase(file);
        }
    } else {
        diff_mode_files[file] = 0;
    }
}

bool is_diff_enabled(std::string file) {
    if (diff_mode_files.find(file) == diff_mode_files.end()) {
        return false;
    }
    return diff_mode_files[file] > 0;
}
