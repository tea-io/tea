#include "diff.h"

#include <algorithm>

std::vector<WriteRequest> diff(const std::string &e, const std::string &f) {
    std::vector<WriteRequest> diffs;
    size_t p = std::mismatch(e.begin(), e.end(), f.begin(), f.end()).first - e.begin();
    size_t s = std::mismatch(e.rbegin(), e.rend(), f.rbegin(), f.rend()).first - e.rbegin();

    if (p + s < e.size()) {
        WriteRequest w;
        w.set_offset(p);
        w.set_data(e.substr(p, e.size() - p - s));
        w.set_size(e.size() - p - s);
        w.set_flag(DELETE);

        diffs.push_back(w);
    }

    if (p + s < f.size()) {
        WriteRequest w;
        w.set_offset(p);
        w.set_data(f.substr(p, f.size() - p - s));
        w.set_size(f.size() - p - s);
        w.set_flag(APPEND);

        diffs.push_back(w);
    }

    return diffs;
}
