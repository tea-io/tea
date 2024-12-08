#include "event.h"
#include "../common/log.h"
#include "../common/tcp.h"

static int cursor_positon(int sock, int id, CursorPosition *req) {
    log(INFO, sock, "[%d] Cursor position: %d, %s", id, req->offset(), req->name().c_str());
    return 0;
}

template <typename T> int empty_handler(int sock, int id, T message) {
    (void)sock;
    (void)id;
    (void)message;
    return 0;
}

recv_handlers get_event_handlers() {
    return {.event{
        .cursor_position = cursor_positon,
        .diff_write_enable = empty_handler<DiffWriteEnable *>,
        .diff_write_disable = empty_handler<DiffWriteDisable *>,
    }};
}
