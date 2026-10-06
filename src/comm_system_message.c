#include "comm_system_message.h"

#include <stdio.h>
#include <string.h>

static void sys_log(const AoeCommSysOps *o, const char *t)
{
    if (o->log) o->log(o->ctx, t);
}

#define CALL(o, f, ...) do { if ((o)->f) (o)->f((o)->ctx, ##__VA_ARGS__); } while (0)

static void reject(const AoeCommSysOps *o, uint32_t dpid)
{
    CALL(o, send_reject, dpid);
}

static void set_player_data(AoeCommSysState *s, const AoeCommSysMessage *m,
                            const AoeCommSysOps *o)
{
    uint32_t slot;
    for (slot = 1; slot <= s->max_players && slot <= AOE_COMM_SYS_MAX_SLOTS; ++slot) {
        if (s->expected_serial[slot] == 0 && s->dpid[slot] != 0) {
            CALL(o, reset_acks, slot);
            s->expected_serial[slot] = 0;
            sys_log(o, "Set expected ser# for player");
        }
    }
    if (s->is_host) return;
    sys_log(o, "Checking state for options rcvd from host");
    if (s->program_state != 3) return;
    sys_log(o, "Group options rcvd from host.");
    if (m->data && m->data_size >= AOE_COMM_SYS_OPTIONS_BYTES) {
        memcpy(s->options, m->data, AOE_COMM_SYS_OPTIONS_BYTES);
        if (s->extra_size + AOE_COMM_SYS_OPTIONS_BYTES <= m->data_size)
            CALL(o, read_extra, m->data + AOE_COMM_SYS_OPTIONS_BYTES, s->extra_size);
        CALL(o, post, 0x17a6);
    }
    s->speed = s->options_speed;
    s->game_stage = s->options_stage;
    CALL(o, find_my_slot);
    if (s->launched) {
        sys_log(o, "Host Launched game");
        CALL(o, touch);
        s->program_state = 5;
        CALL(o, launch_prepare);
        CALL(o, clear_stores);
        memset(s->turn_flags, 0, sizeof s->turn_flags);
        for (slot = 1; slot <= s->max_players && slot <= AOE_COMM_SYS_MAX_SLOTS; ++slot)
            if (s->dpid[slot]) s->expected_serial[slot] = (int32_t)(slot * 2000u);
        s->my_serial = (int32_t)(s->my_slot * 2000u + 1u);
        CALL(o, post, 0x17a6);
        CALL(o, post, 0x17a7);
    } else if (s->my_serial == 0 && s->my_slot != 0) {
        s->my_serial = (int32_t)(s->my_slot * 2000u + 1u);
    }
    CALL(o, refresh);
    CALL(o, post, 0x17a6);
}

static void create_player(AoeCommSysState *s, const AoeCommSysMessage *m,
                          const AoeCommSysOps *o)
{
    uint32_t slot;
    if (s->program_state == 5 || s->game_stage > 6) { reject(o, m->player_id); return; }
    if (!s->is_host) return;
    for (slot = 1; slot <= s->max_players && slot <= AOE_COMM_SYS_MAX_SLOTS; ++slot) {
        if (s->slot_taken[slot] == 0) {
            if (s->program_state == 3) {
                CALL(o, assign_slot, slot, m->player_id, m->create_a, m->create_b);
                s->dpid[slot] = m->player_id;
                s->slot_taken[slot] = 2;
                s->options_dirty = 1;
                s->slot_ready[slot] = 0;
                CALL(o, touch);
                CALL(o, refresh_slot, slot);
                s->options_dirty = 1;
                s->expected_serial[slot] = 0;
                CALL(o, reset_acks, slot);
                CALL(o, share_player_data);
                return;
            }
            break;
        }
    }
    reject(o, m->player_id);
    CALL(o, refresh);
}

static void destroy_player(AoeCommSysState *s, const AoeCommSysMessage *m,
                           const AoeCommSysOps *o)
{
    uint32_t dpid = m->player_id, slot = 0, i;
    if (dpid != 0)
        for (i = s->first_slot; i <= s->last_slot && i <= AOE_COMM_SYS_MAX_SLOTS; ++i)
            if (s->dpid[i] == dpid) { slot = i; break; }
    if (o->slot_playing && o->slot_playing(o->ctx, slot) && s->slot_stopped[slot] == 0) {
        s->slot_dropped[slot] = 1;
        if (slot == s->my_slot) {
            CALL(o, post, 0x17b2);
            for (i = 1; i <= s->max_players && i <= AOE_COMM_SYS_MAX_SLOTS; ++i)
                s->slot_dropped[i] = 1;
            CALL(o, local_dropped);
        } else {
            CALL(o, post_arg, 0x17b3, slot);
        }
    }
    if (s->is_host) {
        sys_log(o, "HOST Send to all - drop player");
        CALL(o, send_drop, slot, dpid, s->game_stage);
        if (slot != 0 && s->slot_stopped[slot] == 0) CALL(o, drop_player, slot, dpid);
    }
}

/* 0x00420a90 */
void aoe_comm_system_message(AoeCommSysState *s, const AoeCommSysMessage *m,
                             const AoeCommSysOps *o)
{
    char buf[48];
    if (!s->analysis_ready) { sys_log(o, "MP in DPANALYSIS"); return; }
    snprintf(buf, sizeof buf, "DP Sys Msg %u", (unsigned)m->type);
    sys_log(o, buf);
    switch (m->type) {
    case AOE_DPSYS_SET_PLAYER_DATA: set_player_data(s, m, o); return;
    case AOE_DPSYS_CREATE_PLAYER: create_player(s, m, o); return;
    case AOE_DPSYS_DESTROY_PLAYER: destroy_player(s, m, o); return;
    case AOE_DPSYS_DELETE_PLAYER_FROM_GROUP: sys_log(o, "delete player of group."); return;
    case AOE_DPSYS_SESSION_LOST:
        sys_log(o, "COMM Host session lost.");
        s->options_dirty = 1;
        CALL(o, refresh);
        CALL(o, post, 0x17a9);
        return;
    case AOE_DPSYS_HOST:
        if (s->program_state != 3 && s->game_stage > 5) {
            sys_log(o, "Assumed host");
            s->is_host = 1;
            s->host_slot = s->my_slot;
            CALL(o, become_host);
            CALL(o, refresh);
            CALL(o, session_state, 0);
            CALL(o, touch);
            CALL(o, post_arg, 0x17b1, s->my_slot);
            CALL(o, host_migrated);
            return;
        }
        sys_log(o, "Host (other) Exit before game start");
        CALL(o, post, 0x17a9);
        CALL(o, session_state, 1);
        return;
    case AOE_DPSYS_SET_PLAYER_NAME:
        CALL(o, refresh);
        sys_log(o, "Set player or group name");
        CALL(o, post, 0x17a6);
        return;
    case AOE_DPSYS_SET_SESSION_DESC: sys_log(o, "Set Sessino Description"); return;
    case AOE_DPSYS_ADD_PLAYER_TO_GROUP:
    case AOE_DPSYS_ADD_GROUP_TO_GROUP: sys_log(o, "Add/delete player of group."); return;
    case AOE_DPSYS_SECURE_MESSAGE: sys_log(o, "Secure message dp"); return;
    case AOE_DPSYS_START_SESSION: sys_log(o, "START SESSION MESSAGE RCVD."); return;
    case AOE_DPSYS_CHAT: sys_log(o, "DP Chat Msg"); return;
    default:
        snprintf(buf, sizeof buf, "COMM Unknown system message: %u", (unsigned)m->type);
        sys_log(o, buf);
        return;
    }
}
