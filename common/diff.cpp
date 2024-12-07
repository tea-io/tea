#include "diff.h"
#include "../lib/dtl/dtl/dtl.hpp"
#include <algorithm>

std::vector<WriteOperation> diff(const std::string &e, const std::string &f, size_t start_offset) {
    std::vector<WriteOperation> diffs;

    dtl::Diff<char, std::string> d(e, f);
    d.compose();

    int offset = -1;
    std::string agg;
    auto s = d.getSes().getSequence();
    auto type = dtl::SES_COMMON;

    for (long unsigned int i = 0; i < s.size(); i++) {
        const auto &elem = s[i];

        if (elem.second.type != type || offset == -1) {
            if (!agg.empty()) {
                WriteOperation w;
                if (type == dtl::SES_ADD) {
                    w.set_flag(APPEND);
                } else if (type == dtl::SES_DELETE) {
                    w.set_flag(DELETE);
                } else {
                    w.set_flag(COMMON);
                }
                w.set_data(agg);
                w.set_offset(offset + start_offset);
                w.set_size(agg.size());
                diffs.push_back(w);
            }

            offset = i;
            agg.clear();
            type = elem.second.type;
        }
        agg += elem.first;
    }

    if (!agg.empty()) {
        WriteOperation w;
        w.set_flag(type == dtl::SES_ADD ? APPEND : DELETE);
        w.set_data(agg);
        w.set_offset(offset + start_offset);
        w.set_size(agg.size());
        diffs.push_back(w);
    }

    return diffs;
}

std::vector<WriteOperation> diff(const std::string &e, const std::string &f) { return diff(e, f, 0); }
