#include "train_action.h"
#include <assert.h>
#include <string.h>

typedef struct { unsigned char bytes[16]; size_t used; int base; } Trace;
static int base(void *context, const AoeTrainAction *action) {
    Trace *trace = context;
    assert(action->action_type == 102);
    ++trace->base;
    return 1;
}
static int write(void *context, const void *bytes, size_t count) {
    Trace *trace = context;
    assert(trace->used + count <= sizeof(trace->bytes));
    memcpy(trace->bytes + trace->used, bytes, count);
    trace->used += count;
    return 1;
}
int main(void) {
    AoeTrainAction action = {.action_type = 102, .unit_id = 83,
                             .progress = 12.5f, .request_id = -1, .prepaid = 1};
    Trace trace = {0};
    AoeTrainActionSerializeOps ops = {&trace, base, write};
    unsigned char expected[11];
    memcpy(expected, &action.unit_id, 2);
    memcpy(expected + 2, &action.progress, 4);
    memcpy(expected + 6, &action.request_id, 4);
    expected[10] = action.prepaid;
    assert(aoe_train_action_serialize_ror(&action, &ops));
    assert(trace.base == 1 && trace.used == sizeof(expected));
    assert(!memcmp(trace.bytes, expected, sizeof(expected)));
    assert(!aoe_train_action_serialize_ror(NULL, &ops));
    return 0;
}
