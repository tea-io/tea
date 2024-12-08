#include "event.h"
#include "../common/io.h"
#include "../common/log.h"

static int cursor_positon(int sock, int id, CursorPosition *req) {
    log(INFO, sock, "[%d] Cursor position: %d, %s", id, req->offset(), req->name().c_str());
    return 0;
}

recv_handlers get_event_handlers() { return {.event{.cursor_position = cursor_positon}}; }
