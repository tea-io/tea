#include "fs.h"
#include "../proto/messages.pb.h"

static int init_request(int sock, int id, InitRequest *req) {
    (void)req;
    InitResponse res;
    res.set_error(0);
    int err = send_message(sock, id, Type::INIT_RESPONSE, &res);
    if (err < 0) {
        return -1;
    }
    return 0;
}

template <typename T> int respons_handler(int sock, int id, T message) {
    (void)sock;
    (void)id;
    (void)message;
    return 0;
}

recv_handlers get_handlers() {
    return recv_handlers{
        .init_request = init_request,
        .init_response = respons_handler<InitResponse *>,
    };
}
