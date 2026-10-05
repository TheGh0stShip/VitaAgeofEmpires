#ifndef VITA_AOE_COMM_HANDLER_H
#define VITA_AOE_COMM_HANDLER_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a communications handler (0x0041df70..
   0x00425b20; the game's +0x18c, global 0x00580da8): the multiplayer
   players' slots and states, the turn, pause and resume, the names, the
   command and chat sends, the fast (unguaranteed) pings, and the
   guaranteed-delivery stores that hold sent messages until every player
   acknowledged them and received ones until they can run in order.
   DirectPlay (global 0x00580eac), the message builder (0x00422680), the
   game window and the log are ops. Player slots are 1..count (at most
   9). Semantic storage; offsets noted. */

#define AOE_COMM_SLOTS 10
#define AOE_COMM_TX_SLOTS 0x1f6
#define AOE_COMM_RX_SLOTS 0x1f6

/* A sent message kept until acknowledged (0x38 bytes each). */
typedef struct {
    uint32_t time;                    /* +0x00 */
    int32_t serial;                   /* +0x04: 0 free */
    uint8_t *data;                    /* +0x08 */
    uint32_t size;                    /* +0x0c */
    int32_t pending[AOE_COMM_SLOTS];  /* +0x10: per player, an ack missing */
} AoeCommTxRor;

/* A received message waiting for its turn in order (0x18 bytes each). */
typedef struct {
    uint8_t *data;                    /* +0x00 */
    int32_t serial;                   /* +0x04 */
    uint32_t size;                    /* +0x08: 0 free */
    int32_t from;                     /* +0x0c: the sender's DirectPlay ID */
    int32_t extra;                    /* +0x10 */
} AoeCommRxRor;

typedef struct {
    void *context;
    /* DirectPlay: present, our ID (0x005817bc), send (+0x68; 0 success),
       set our player data (+0x74, guaranteed 2). */
    int (*dp_present)(void *context);
    int32_t (*self_dpid)(void *context);
    int32_t (*dp_send)(void *context, int32_t to, const void *data, uint32_t size);
    int32_t (*dp_set_player_data)(void *context, const void *data, uint32_t size);
    /* 0x00422680(to 0, kind, data, size, 0): a game message to everyone. */
    int32_t (*message)(void *context, uint8_t kind, const void *data, uint32_t size);
    /* 0x00421480 chat (from, the 'Y' / 'N' recipients by slot, text). */
    void (*chat)(void *context, int32_t from, const char recipients[10], const char *text);
    /* 0x004200e0: PostMessage(window +0x130c, WM_USER, w, l). */
    void (*post)(void *context, uint32_t w, uint32_t l);
    /* 0x0042c330 log; 0x0041d700 an error report. */
    void (*log)(void *context, const char *text);
    void (*report)(void *context, int32_t hr, const char *what);
    /* Others: 0x00420820 drop a player, 0x00424f30 refresh a slot,
       0x0041f780 run a received message, 0x0041eaa0 queue a local
       command (0x00428090), the two queues' flush (0x00427f30), 0x00427fd0
       a player's queue flushed, 0x004239b0, string (game +0x20), clock
       and random numbers (0x00526053 / 0x0052605d). */
    void (*drop)(void *context, uint32_t player, int32_t dpid);
    void (*refresh)(void *context, uint32_t player, int flag);
    void (*run)(void *context, int32_t size, const uint8_t *data, int32_t from, int32_t extra,
                int flag);
    void (*queue_local)(void *context, uint32_t turn, const void *data, uint32_t size,
                        uint32_t player, uint8_t seq, int32_t extra);
    void (*flush_queues)(void *context);
    void (*flush_player_queue)(void *context, uint32_t player);
    int (*call_4239b0)(void *context);
    void (*string)(void *context, int32_t id, char *out, int size);
    uint32_t (*now_ms)(void *context);
    void (*srand)(void *context, uint32_t seed);
    int32_t (*rand)(void *context);
    /* The player data block (+0x1550, 0x1d0 bytes) the host shares. */
    void (*state_block)(void *context, uint8_t out[0x1d0]);
} AoeCommOpsRor;

typedef struct {
    int32_t is_host;                  /* +0x1c */
    int32_t field_20;                 /* +0x20 */
    uint32_t turn;                    /* +0x10c8 */
    uint32_t self;                    /* +0x10e0: our slot */
    int32_t multiplayer;              /* +0x10e4 */
    char name[0x81];                  /* +0x10f8 */
    char info[0x81];                  /* +0x1179 */
    char label[0x100];                /* +0x11fa: "name [info]" */
    char text[0x100];                 /* +0xfc5: report text */
    int32_t field_12a8;               /* +0x12a8 */
    uint32_t bytes_sent;              /* +0x12c0 */
    char (*names)[0x80];              /* +0x12c8: per slot */
    int32_t status;                   /* +0x12d4 */
    uint16_t count;                   /* +0x12d8: players */
    uint32_t guid_12dc[4];            /* +0x12dc */
    uint32_t guid_12fc[4];            /* +0x12fc */
    uint32_t guid_1318[4];            /* +0x1318: the null GUID */
    uint32_t window;                  /* +0x130c */
    uint32_t time_1344[AOE_COMM_SLOTS];   /* +0x1344 */
    uint32_t fast_time[AOE_COMM_SLOTS];   /* +0x1394 */
    int32_t stop_turn[AOE_COMM_SLOTS];    /* +0x13bc */
    int32_t ack_serial[AOE_COMM_SLOTS];   /* +0x13e4 */
    uint32_t ack_time[AOE_COMM_SLOTS];    /* +0x140c */
    int32_t serial_1434[AOE_COMM_SLOTS];  /* +0x1434 */
    int32_t serial_145c[AOE_COMM_SLOTS];  /* +0x145c */
    int32_t caps[6];                  /* +0x1484..+0x1498 */
    uint8_t gtd;                      /* +0x1514: guaranteed delivery on */
    int32_t tx_pending;               /* +0x151c */
    AoeCommTxRor *tx;                 /* +0x1520 */
    AoeCommRxRor *rx;                 /* +0x1524 */
    int32_t rx_serial[AOE_COMM_SLOTS];/* +0x1528: the last run per slot */
    uint8_t dirty;                    /* +0x1550 */
    uint32_t data_time;               /* +0x1554 */
    int32_t state;                    /* +0x1558: 3 lobby, 4 paused, 5 running */
    int32_t dpid[AOE_COMM_SLOTS];     /* +0x1564 */
    uint8_t lead;                     /* +0x1590 */
    int32_t stat[8][AOE_COMM_SLOTS];  /* +0x1594..+0x16ac */
    int32_t player_state[AOE_COMM_SLOTS]; /* +0x16d4: 2 human, 4 computer, 5 */
    uint8_t ready[AOE_COMM_SLOTS];    /* +0x16fc */
    int32_t seed;                     /* +0x170c */
    uint16_t high, low;               /* +0x1710, +0x1712: the active slot range */
    uint32_t data_size;               /* +0x171c */
    uint8_t *data;                    /* +0x04 */
    uint32_t last_turn[AOE_COMM_SLOTS];   /* +0x1720 */
    char status_text[0x20];           /* 0x00580e74 "Ready=..." */
    /* 0x005817d8 / 0x005817dc: the command sequence of the turn. */
    uint8_t cmd_seq;
    int32_t cmd_turn;
    const AoeCommOpsRor *ops;
} AoeCommRor;

/* Slots: 0x00423da0 a playing human (state 2 or 5), 0x00423de0 a
   computer (4); 0x00423d40 a slot's state set (refreshed, posted 0x17a6;
   a lobby host resends its data); 0x0041e130 the active range (+0x1710 the
   highest, +0x1712 the lowest playing or computer slot); 0x00424330 a
   slot initialised for a DirectPlay ID (its stats, state and serials
   cleared; its queue flushed); 0x00425330 / 0x004253b0 its stats;
   0x00425440 every playing human has stat 0; 0x00423c10 our slot;
   0x00423c20 host (always without multiplayer). */
int aoe_comm_playing_ror(const AoeCommRor *c, uint32_t slot);
int aoe_comm_computer_ror(const AoeCommRor *c, uint32_t slot);
void aoe_comm_set_player_state_ror(AoeCommRor *c, uint32_t slot, int32_t state);
int aoe_comm_active_range_ror(AoeCommRor *c);
void aoe_comm_init_player_ror(AoeCommRor *c, uint32_t slot, int32_t dpid);
int aoe_comm_stats_ror(const AoeCommRor *c, uint32_t slot, int32_t out[8]);
int32_t aoe_comm_stat0_ror(const AoeCommRor *c, uint32_t slot);
int aoe_comm_all_stat0_ror(const AoeCommRor *c);
uint32_t aoe_comm_self_ror(const AoeCommRor *c);
int32_t aoe_comm_is_host_ror(const AoeCommRor *c);

/* Status: 0x0041dfe0 a slot's status string (0x4b1 computer, 0x4b2
   gone, 0x4b3 not started, 0x4b4 behind, 0x4b0 ok; the text into a
   0x20-byte buffer); 0x0041dfa0 "[slot] name - status" (the name
   0x004241a0 given); 0x00423c40 "Ready=" with a letter per slot (A in
   step, else by state: 0 '-', 2 'W', 4 'C', 5 'X', 6 'V', else '?');
   0x00423870 the link level (15 / 14 / 13 running, 12 dirty, 9 player,
   7 host, 6 / 5 session / GUID, 4 / 3 / 2 / 1). */
const char *aoe_comm_status_text_ror(AoeCommRor *c, uint32_t slot, char out[0x20]);
const char *aoe_comm_slot_label_ror(AoeCommRor *c, uint32_t slot, const char *name);
const char *aoe_comm_ready_text_ror(AoeCommRor *c);
int32_t aoe_comm_link_level_ror(const AoeCommRor *c);

/* Pause and resume: 0x00423a20 paused (state 4, posted 0x17a2);
   0x00423a00 running (5, 0x17a3); 0x00423a40 the host's request (0x50
   pause / 0x55 resume); 0x00423ac0 pause (alone at once; a host tells
   everyone; a guest asks with 0x57); 0x00423b20 resume (0x2b to ask).
   0x00423b90 the host's seed (-1: random from the clock), the data
   resent. */
void aoe_comm_paused_ror(AoeCommRor *c, uint32_t by);
void aoe_comm_running_ror(AoeCommRor *c, uint32_t by);
void aoe_comm_request_ror(AoeCommRor *c, int pause);
void aoe_comm_pause_ror(AoeCommRor *c);
void aoe_comm_resume_ror(AoeCommRor *c);
void aoe_comm_set_seed_ror(AoeCommRor *c, int32_t seed);

/* Names: 0x00424440 ours (the info "Optional Information"); 0x004244a0 a
   slot's (ours through 0x00424440); 0x00424410 "name [info]";
   0x004244f0 the window; 0x00424eb0 the session GUID. */
void aoe_comm_set_own_name_ror(AoeCommRor *c, const char *name);
void aoe_comm_set_name_ror(AoeCommRor *c, uint32_t slot, const char *name);
int aoe_comm_format_label_ror(AoeCommRor *c, const char *name);
void aoe_comm_set_window_ror(AoeCommRor *c, uint32_t window);
void aoe_comm_set_guid_ror(AoeCommRor *c, const uint32_t guid[4]);

/* Turns and commands: 0x0041df70 the turn back to 0 and both queues
   flushed; 0x0041eb30 a command (multiplayer: message 0x3e; alone: queued
   for turn + lead with the next sequence, 0x0041eb10 without the extra);
   0x00422610 the next command sequence of a turn (1 for a new turn; 0 at
   255; at most 100 while paused); 0x00421640 message 0x53 (two shorts);
   0x00421680 the turn markers (0x51, then 0x44 for the next three
   turns); 0x00421770 every playing slot has reached the turn; 0x00421800
   a checksum (0x4d: nine dwords with the turn and time); 0x004253e0 /
   0x00425410 past the start (turn 7). */
void aoe_comm_reset_turn_ror(AoeCommRor *c);
int aoe_comm_send_command_ror(AoeCommRor *c, const void *data, uint32_t size, int32_t extra,
                              uint8_t (*seq)(void *context, uint32_t turn));
uint8_t aoe_comm_command_seq_ror(AoeCommRor *c, int32_t turn);
void aoe_comm_send_53_ror(AoeCommRor *c, int16_t a, int16_t b);
void aoe_comm_send_turn_markers_ror(AoeCommRor *c, uint8_t a, uint8_t b);
int aoe_comm_caught_up_ror(const AoeCommRor *c);
int32_t aoe_comm_send_checksum_ror(AoeCommRor *c, int32_t a, int32_t b, int32_t d, int32_t e,
                                   int32_t f, int32_t g, int32_t h);
int aoe_comm_started_ror(const AoeCommRor *c);
int aoe_comm_started_or_tick_ror(AoeCommRor *c, void (*tick)(void *context, uint32_t v),
                                 void *context);

/* Chat: 0x00421370 to the players flagged by `to_slot` (0x0041ca70),
   0x004213e0 to everyone, 0x00421420 to one slot. */
void aoe_comm_chat_some_ror(AoeCommRor *c, const char *text, int (*to_slot)(void *context,
                                                                             uint32_t slot));
void aoe_comm_chat_all_ror(AoeCommRor *c, const char *text);
void aoe_comm_chat_one_ror(AoeCommRor *c, int32_t from, int32_t slot, const char *text);

/* Fast pings: 0x0041f660 a ping (0x31, the time and turn) to one slot (or
   all, resetting their times); 0x0041f5e0 the host pings slots silent for
   8 s (or all when forced); 0x0041f6e0 the unguaranteed send (not to
   ourselves). 0x00423970 every slot's ping times now. */
int aoe_comm_ping_ror(AoeCommRor *c, uint32_t slot);
void aoe_comm_ping_quiet_ror(AoeCommRor *c, int force);
int32_t aoe_comm_fast_send_ror(AoeCommRor *c, int32_t to, const void *data, uint32_t size);
void aoe_comm_reset_ping_times_ror(AoeCommRor *c);

/* Players leaving: 0x00420510 a slot's stop turn; 0x00420530 slots whose
   stop turn is `turn` resign (state 3, posted 0x17b4); past turn 6 with
   no stops and nobody else left, 0x17bc is posted; 0x004256c0 a slot
   dropped (0x4b to everyone, unless stopping; 0x17a4; an early host
   resends); 0x004239c0 the host drops everyone before turn 7;
   0x00420780 our slot found by our DirectPlay ID (0x17b8). */
void aoe_comm_set_stop_turn_ror(AoeCommRor *c, uint32_t slot, int32_t turn);
void aoe_comm_check_stop_turn_ror(AoeCommRor *c, int32_t turn);
int aoe_comm_drop_player_ror(AoeCommRor *c, uint32_t slot);
void aoe_comm_drop_early_ror(AoeCommRor *c);
void aoe_comm_find_self_ror(AoeCommRor *c);
/* 0x00420640: the host shares its player data (the 0x1d0-byte block and
   its extra data): when forced or every 2 s while dirty; 0x17bb posted,
   the ready flags cleared. 0x00420630 clears the dirty flag; 0x004258a0
   frees the extra data. */
int32_t aoe_comm_share_data_ror(AoeCommRor *c, int force);
void aoe_comm_clean_ror(AoeCommRor *c);
void aoe_comm_free_data_ror(AoeCommRor *c);

/* Guaranteed delivery: 0x0041f260 / 0x0041f2b0 one sent / received slot
   emptied; 0x00421a20 both stores emptied; 0x004219d0 the serials
   cleared; 0x00421ab0 a sender's received messages dropped; 0x00421b30 a
   slot's acks cleared (0x00421b60 both for a slot); 0x00421b10 the sent
   messages waiting; 0x00421c80 a sent message kept with the slots that
   must acknowledge it; 0x00421b80 a received one kept (once); 0x00421d90
   an ack ('A', 8 bytes) to a slot; 0x00422550 a resend request ('X') at
   most every 0.5 s per serial; 0x00421e40 an ack received;
   0x00421ec0 fully acknowledged messages dropped, the rest counted;
   0x00421890 the received messages run in order per sender. */
void aoe_comm_tx_clear_ror(AoeCommTxRor *t);
void aoe_comm_rx_clear_ror(AoeCommRxRor *r);
void aoe_comm_gtd_reset_ror(AoeCommRor *c);
void aoe_comm_clear_serials_ror(AoeCommRor *c);
void aoe_comm_drop_rx_from_ror(AoeCommRor *c, uint32_t slot);
void aoe_comm_clear_acks_ror(AoeCommRor *c, uint32_t slot);
void aoe_comm_forget_slot_ror(AoeCommRor *c, uint32_t slot);
int32_t aoe_comm_tx_count_ror(const AoeCommRor *c);
int aoe_comm_tx_store_ror(AoeCommRor *c, int32_t serial, const void *data, uint32_t size,
                          const int32_t *pending);
int aoe_comm_rx_store_ror(AoeCommRor *c, int32_t serial, const void *data, uint32_t size,
                          int32_t from, int32_t extra);
int aoe_comm_send_ack_ror(AoeCommRor *c, int32_t serial, uint32_t slot);
int aoe_comm_request_resend_ror(AoeCommRor *c, int32_t serial, uint32_t slot);
int aoe_comm_ack_received_ror(AoeCommRor *c, int32_t serial, uint32_t slot);
int aoe_comm_tx_purge_ror(AoeCommRor *c);
int aoe_comm_rx_deliver_ror(AoeCommRor *c);

#endif

#ifndef VITA_AOE_COMM_HANDLER_DP_H
#define VITA_AOE_COMM_HANDLER_DP_H

/* DirectPlay sessions and the lobby (0x00424500..0x00425e30). */
typedef struct {
    void *context;
    /* DirectPlay: create (Ordinal 1 and the interface; 0 success),
       release (the count left), close the session, the caps (twice),
       the session description (its GUID and player count), create our
       player (our ID), destroy it, our player's caps (three dwords), a
       player's data (0 success). */
    int32_t (*create)(void *context, const uint32_t guid[4]);
    int32_t (*release)(void *context);
    int32_t (*close)(void *context);
    int32_t (*caps)(void *context);
    int32_t (*session)(void *context, uint32_t guid[4], int32_t *players);
    int32_t (*create_player)(void *context, const char *name, const char *info, int32_t *id);
    int32_t (*destroy_player)(void *context, int32_t id);
    int32_t (*player_caps)(void *context, int32_t id, int32_t out[3]);
    int32_t (*player_data)(void *context, int32_t id);
    void (*set_self_dpid)(void *context, int32_t id);
    /* The command queue's take (first / for a turn). */
    uint8_t *(*take_first)(void *context);
    uint8_t *(*take_turn)(void *context, uint32_t turn);
    /* 0x00422ce0 one unlink step (the new level). */
    int32_t (*unlink_step)(void *context);
} AoeCommDpOpsRor;

/* 0x004255e0 close the session (the GUID and turns cleared); 0x00425660
   release DirectPlay (the GUID cleared); 0x00425560 the caps (guaranteed
   delivery on); 0x004254a0 the session description (+0x12fc, +0x127c);
   0x004257a0 our player destroyed; 0x00424dd0 our caps (+0x1484..);
   0x004249a0 our player created (failing closes and releases, state 1);
   0x00424500 DirectPlay created (not twice); 0x00424ea0 present;
   0x00424ee0 every active slot refreshed (0x17a4). */
int32_t aoe_comm_dp_close_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp);
int32_t aoe_comm_dp_release_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp);
int32_t aoe_comm_dp_caps_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp);
int32_t aoe_comm_dp_session_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp, int32_t *players);
int32_t aoe_comm_dp_release_player_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp);
int32_t aoe_comm_dp_player_caps_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp);
int aoe_comm_dp_add_self_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp);
int32_t aoe_comm_dp_create_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp);
int aoe_comm_dp_present_ror(const AoeCommRor *c);
void aoe_comm_refresh_all_ror(AoeCommRor *c);
/* 0x0041e9c0: playing slots whose data cannot be read are dropped
   (0x4b to everyone; dropped unless stopping). 0x0041ead0: the next
   command to run (alone or in the lobby: the first queued; else the one
   for this turn). 0x004237c0: unlink to a level (at least 1), step by
   step until there or stuck. */
void aoe_comm_check_players_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp);
uint8_t *aoe_comm_next_command_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp);
int32_t aoe_comm_unlink_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp, int32_t level);

/* 0x0041f2e0: with guaranteed delivery on, the receive log line "GTD:
   RX Ser#%d From P#%d T#%ld ToExecT#%d Cmd=%d '%c' Seq=%d" (the command
   byte both as a number and a character). 0x004200d0: a post with 0 as
   the second value. */
void aoe_comm_gtd_rx_log_ror(const AoeCommRor *c, const AoeCommOpsRor *ops, int32_t serial,
                             int32_t from, int32_t exec_turn, uint8_t command, uint8_t sequence);
void aoe_comm_post_ror(const AoeCommOpsRor *ops, uint32_t w);

/* The lobby (0x00425a10..0x00425e30). */
typedef struct {
    uint32_t window;      /* +0x00 */
    void *iface;          /* +0x08 */
    int32_t field_c;      /* +0x0c */
    const uint8_t *settings; /* +0x10: the connection settings */
    uint8_t launched;     /* +0x14 */
    int has_poster;       /* +0x18 */
} AoeLobbyRor;
typedef struct {
    void *context;
    void (*release_iface)(void *context, void *iface);
    int32_t (*send)(void *context, void *iface, const uint8_t *data, uint32_t size);
    void (*free_settings)(void *context, const uint8_t *settings);
} AoeLobbyOpsRor;
/* 0x00425a10 constructor (with a window poster); 0x00425a80 destructor;
   0x00425ab0 released (settings freed, the interface released, not
   launched); 0x00425af0 launched (also 0x004241e0); 0x00425dc0 hosting
   (settings flags bit 1); 0x00425de0 the player name pointer; 0x00425e00
   the 80-byte session block copied; 0x00425e30 a lobby message (a 0x30
   header: 5, 0, the game GUID, four values, the size; then the data).
   0x00425b20: the enumeration callback (logs, 1). */
void aoe_lobby_init_ror(AoeLobbyRor *l, uint32_t window);
void aoe_lobby_release_ror(AoeLobbyRor *l, const AoeLobbyOpsRor *ops);
void aoe_lobby_destroy_ror(AoeLobbyRor *l, const AoeLobbyOpsRor *ops);
uint8_t aoe_lobby_launched_ror(const AoeLobbyRor *l);
uint32_t aoe_lobby_hosting_ror(const AoeLobbyRor *l, uint32_t flags);
int aoe_lobby_name_ror(const AoeLobbyRor *l, const char *name, const char **out);
int aoe_lobby_session_ror(const AoeLobbyRor *l, const uint8_t session[80], uint8_t out[80]);
int32_t aoe_lobby_send_ror(AoeLobbyRor *l, const AoeLobbyOpsRor *ops, const uint32_t guid[4],
                           const void *data, uint32_t size, int32_t a, int32_t b, int32_t c,
                           int32_t d);
int aoe_lobby_enum_player_ror(int32_t dpid, const char *name, uint8_t flags);
/* 0x004258d0(iface, enable): a NULL interface gives E_INVALIDARG
   (0x80070057); else its IDirectPlay ({5454e9a0-db65-11ce-921c-
   00aa006c4972}, a failed QueryInterface returned as is) takes
   EnableNewPlayers (+0x28) with `enable`, is released, and "Released to
   links=%d" logs the references left. The EnableNewPlayers result. */
typedef struct {
    void *context;
    int32_t (*query_dplay)(void *context, void *iface, void **dplay);
    int32_t (*enable_new_players)(void *context, void *dplay, int32_t enable);
    int32_t (*release)(void *context, void *dplay);
    void (*log)(void *context, const char *line);
} AoeCommEnableOpsRor;
int32_t aoe_comm_enable_new_players_ror(void *iface, int32_t enable,
                                        const AoeCommEnableOpsRor *ops);
/* 0x00425930: the game options to the lobby zone: unless launched it
   logs "NOT LOBBY LAUNCHED.  EXITING" and gives 0; else it logs "Sending
   zone message.", sends (the game's +0x8f8..+0x904 values) and on a
   failure reports it as "SendGameOptions" (0). */
int aoe_lobby_send_options_ror(AoeLobbyRor *l, const AoeLobbyOpsRor *ops, const uint32_t guid[4],
                               const void *data, uint32_t size, const int32_t values[4],
                               const AoeCommOpsRor *comm);

#endif
