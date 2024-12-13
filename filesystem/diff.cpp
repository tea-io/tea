#include "diff.h"
#include <dtl/dtl.hpp>

std::map<std::string, short> diff_mode_files;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-null-dereference"
std::vector<WriteRequest> diff(const std::string &e, const std::string &f) {
    std::vector<WriteRequest> diffs;

    dtl::Diff<char, std::string> d(e, f);
    d.compose();

    int offset = -1;
    std::string agg;
    auto s = d.getSes().getSequence();
    auto type = dtl::SES_COMMON;

    for (unsigned long int i = 0; i < s.size(); i++) {
        const auto &elem = s[i];

        if (elem.second.type == dtl::SES_COMMON) {
            continue;
        }

        if (elem.second.type != type || offset == -1) {
            if (!agg.empty()) {
                WriteRequest wr = WriteRequest();
                wr.set_flag(type == dtl::SES_ADD ? APPEND : DELETE);
                wr.set_data(agg);
                wr.set_offset(offset);
                wr.set_size(agg.size());
                diffs.push_back(wr);
            }

            offset = i;
            agg.clear();
            type = elem.second.type;
        }
        agg += elem.first;
    }

    if (!agg.empty()) {
        WriteRequest wr = WriteRequest();
        wr.set_flag(type == dtl::SES_ADD ? APPEND : DELETE);
        wr.set_data(agg);
        wr.set_offset(offset);
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
