#include "../common/log.h"
#include "../proto/messages.pb.h"
#include "tcp.h"
#include <string>
#include <thread>

std::string banner = R"(
 _                __
| |_ ___  __ _   / _|___
| __/ _ \/ _` | | |_/ __|
| ||  __/ (_| | |  _\__ \
 \__\___|\__,_| |_| |___/
)";

int main() {
    set_debug_log(true);
    log(NONE, banner.c_str());
    int sock = connect("127.0.0.1", 5210);
    if (sock < 0) {
        return 1;
    }
    std::thread t(recv_thread, sock);

    InitRequest req = InitRequest();
    req.set_error(0);
    InitResponse res;
    int err = request_response<InitResponse>(sock, req, &res);
    if (err < 0) {
        log(ERROR, sock, "Error sending message");
        return 1;
    } else {
        log(INFO, sock, "Received response: %d", res.error());
    }
    t.detach();
    close(sock);
    return 0;
};
