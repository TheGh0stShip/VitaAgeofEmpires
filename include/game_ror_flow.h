#ifndef AOE_GAME_ROR_FLOW_H
#define AOE_GAME_ROR_FLOW_H

#include <stdint.h>

/* Tribe (Rise of Rome 1.0a) game-object flow helpers. */

typedef struct GameRorFlowStringOps {
    void *ctx;
    /* vtable +0x20: load string resource `id` into buf (len bytes). */
    char *(*load_string)(void *ctx, int32_t id, char *buf, int32_t len);
    /* 0x004174e0 base formatter (see game_startup.h). */
    char *(*base_format)(void *ctx, int32_t kind, int32_t index,
                         int32_t sub, char *buf, int32_t len);
} GameRorFlowStringOps;

/* Resolve (kind, index, sub) to a string id; returns -1 when the tribe
 * table has no entry. *defer_to_base is set when the base formatter owns
 * the pair; *terminate when the original forces buf[len-1] = 0. */
int32_t game_ror_flow_message_string_id(int32_t kind, int32_t index,
                                        int32_t sub, int *defer_to_base,
                                        int *terminate);

/* 0x004ff580: tribe game message-string formatter (vtable +0x28). */
char *game_ror_flow_format_message(const GameRorFlowStringOps *ops,
                                   int32_t kind, int32_t index, int32_t sub,
                                   char *buf, int32_t len);

#endif
