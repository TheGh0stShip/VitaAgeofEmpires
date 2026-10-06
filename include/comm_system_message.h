#ifndef AOE_COMM_SYSTEM_MESSAGE_H
#define AOE_COMM_SYSTEM_MESSAGE_H

#include <stdint.h>

/* 0x00420a90: the communications handler's DirectPlay system-message
   dispatcher (RoR 1.0a).  The handler's fields it reads and writes are
   modelled semantically here; everything it reaches through other
   handler methods goes through AoeCommSysOps. Slots are 1-based. */

#define AOE_COMM_SYS_MAX_SLOTS 9
#define AOE_COMM_SYS_OPTIONS_BYTES 0x1d0u

enum {
    AOE_DPSYS_CREATE_PLAYER = 0x0003,
    AOE_DPSYS_DESTROY_PLAYER = 0x0005,
    AOE_DPSYS_ADD_PLAYER_TO_GROUP = 0x0007,
    AOE_DPSYS_DELETE_PLAYER_FROM_GROUP = 0x0021,
    AOE_DPSYS_SESSION_LOST = 0x0031,
    AOE_DPSYS_HOST = 0x0101,
    AOE_DPSYS_SET_PLAYER_DATA = 0x0102,
    AOE_DPSYS_SET_PLAYER_NAME = 0x0103,
    AOE_DPSYS_SET_SESSION_DESC = 0x0104,
    AOE_DPSYS_ADD_GROUP_TO_GROUP = 0x0105,
    AOE_DPSYS_SECURE_MESSAGE = 0x0107,
    AOE_DPSYS_START_SESSION = 0x0108,
    AOE_DPSYS_CHAT = 0x0109
};

typedef struct AoeCommSysMessage {
    uint32_t type;
    uint32_t player_id;         /* +0x08: create/destroy DirectPlay ID */
    const uint8_t *data;        /* +0x0c: set-player-data block */
    uint32_t data_size;         /* +0x10 */
    uint32_t create_a, create_b;/* +0x20 / +0x24 of a create message */
} AoeCommSysMessage;

typedef struct AoeCommSysState {
    int32_t analysis_ready;     /* +0x10e4: 0 = "MP in DPANALYSIS" */
    int32_t is_host;            /* +0x1c */
    int32_t program_state;      /* +0x1558: 3 lobby, 5 launched */
    uint32_t game_stage;        /* +0x10c8 */
    uint32_t my_slot;           /* +0x10e0 */
    uint16_t max_players;       /* +0x12d8 */
    uint16_t first_slot;        /* +0x1712 */
    uint16_t last_slot;         /* +0x1710 */
    uint32_t dpid[AOE_COMM_SYS_MAX_SLOTS + 1];          /* +0x1564 */
    int32_t expected_serial[AOE_COMM_SYS_MAX_SLOTS + 1];/* +0x1528 */
    int32_t slot_taken[AOE_COMM_SYS_MAX_SLOTS + 1];     /* +0x16d4 */
    uint8_t slot_ready[AOE_COMM_SYS_MAX_SLOTS + 1];     /* +0x16fc */
    int32_t slot_stopped[AOE_COMM_SYS_MAX_SLOTS + 1];   /* +0x13bc */
    int32_t slot_dropped[AOE_COMM_SYS_MAX_SLOTS + 1];   /* +0x174c */
    uint32_t turn_flags[9];     /* +0x13c0 cleared on launch */
    uint8_t options[AOE_COMM_SYS_OPTIONS_BYTES];        /* +0x1550 */
    uint8_t options_dirty;      /* +0x1550 byte */
    int32_t launched;           /* +0x1718 */
    uint32_t extra_size;        /* +0x171c */
    uint32_t host_slot;         /* +0x1714 */
    uint32_t options_stage;     /* +0x155c -> game_stage */
    uint32_t options_speed;     /* +0x158c -> speed */
    uint32_t speed;             /* +0x10dc */
    int32_t my_serial;          /* +0x14e4 */
} AoeCommSysState;

typedef struct AoeCommSysOps {
    void *ctx;
    void (*log)(void *ctx, const char *text);
    void (*reset_acks)(void *ctx, uint32_t slot);           /* 0x00421b60 */
    void (*read_extra)(void *ctx, const uint8_t *data, uint32_t size); /* 0x004257f0 */
    void (*post)(void *ctx, uint32_t msg);                  /* 0x004200d0 */
    void (*post_arg)(void *ctx, uint32_t msg, uint32_t arg);/* 0x004200e0 */
    void (*find_my_slot)(void *ctx);                        /* 0x00420780 */
    void (*touch)(void *ctx);                               /* 0x0041e130 */
    void (*launch_prepare)(void *ctx);                      /* 0x00423970 */
    void (*clear_stores)(void *ctx);                        /* 0x00421a20 */
    void (*refresh)(void *ctx);                             /* 0x00424ee0 */
    void (*send_reject)(void *ctx, uint32_t dpid);          /* 0x0041f6e0 'I' */
    void (*assign_slot)(void *ctx, uint32_t slot, uint32_t dpid,
                        uint32_t a, uint32_t b);            /* 0x00424330 */
    void (*refresh_slot)(void *ctx, uint32_t slot);         /* 0x00424f30 */
    void (*share_player_data)(void *ctx);                   /* 0x00420640 */
    int (*slot_playing)(void *ctx, uint32_t slot);          /* 0x00423da0 */
    void (*local_dropped)(void *ctx);                       /* local player dropped */
    void (*send_drop)(void *ctx, uint32_t slot, uint32_t dpid,
                      uint32_t stage);                      /* 0x00422680 0x4b */
    void (*drop_player)(void *ctx, uint32_t slot, uint32_t dpid); /* 0x00420820 */
    void (*become_host)(void *ctx);                         /* 0x0041f5e0(1) */
    void (*session_state)(void *ctx, int lost);             /* 0x004258d0 */
    void (*host_migrated)(void *ctx);                       /* 0x0041e9c0 */
} AoeCommSysOps;

void aoe_comm_system_message(AoeCommSysState *s, const AoeCommSysMessage *m,
                             const AoeCommSysOps *ops);

#endif
