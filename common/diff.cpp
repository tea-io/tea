#include "diff.h"
#include "../lib/dtl/dtl/dtl.hpp"

#include <algorithm>

std::vector<WriteRequest> diff(const std::string &e, const std::string &f) {
    std::vector<WriteRequest> diffs;

    dtl::Diff<char, std::string> d(e, f);
    d.compose();

    int offset = -1;
    std::string agg;
    auto s = d.getSes().getSequence();
    auto type = dtl::SES_COMMON;

    for (int i = 0; i < s.size(); i++) {
        const auto &elem = s[i];

        if (elem.second.type == dtl::SES_COMMON) {
            continue;
        }

        if (elem.second.type != type || offset == -1) {
            if (!agg.empty()) {
                WriteRequest wr;
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
        WriteRequest wr;
        wr.set_flag(type == dtl::SES_ADD ? APPEND : DELETE);
        wr.set_data(agg);
        wr.set_offset(offset);
        wr.set_size(agg.size());
        diffs.push_back(wr);
    }

    return diffs;
}
