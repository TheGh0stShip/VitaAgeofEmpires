#include "comm_system_message.h"

#include <assert.h>
#include <string.h>

typedef struct { int posts[8]; int nposts; int rejects; int assigned; int drops; int lost; } Rec;
static void post(void *c, uint32_t m) { Rec *r = c; r->posts[r->nposts++ & 7] = (int)m; }
static void post_arg(void *c, uint32_t m, uint32_t a) { (void)a; post(c, m); }
static void rej(void *c, uint32_t d) { (void)d; ((Rec *)c)->rejects++; }
static void asg(void *c, uint32_t s, uint32_t d, uint32_t a, uint32_t b)
{ (void)d; (void)a; (void)b; ((Rec *)c)->assigned = (int)s; }
static int playing(void *c, uint32_t s) { (void)c; return s != 0; }
static void drop(void *c, uint32_t s, uint32_t d) { (void)s; (void)d; ((Rec *)c)->drops++; }
static void sess(void *c, int l) { ((Rec *)c)->lost = l; }

int main(void)
{
    Rec r; AoeCommSysState s; AoeCommSysMessage m; AoeCommSysOps o;
    uint8_t blk[AOE_COMM_SYS_OPTIONS_BYTES];
    memset(&o, 0, sizeof o); o.ctx = &r; o.post = post; o.post_arg = post_arg;
    o.send_reject = rej; o.assign_slot = asg; o.slot_playing = playing;
    o.drop_player = drop; o.session_state = sess;

    /* not ready: nothing happens */
    memset(&r, 0, sizeof r); memset(&s, 0, sizeof s); memset(&m, 0, sizeof m);
    m.type = AOE_DPSYS_SESSION_LOST; aoe_comm_system_message(&s, &m, &o);
    assert(r.nposts == 0);
    s.analysis_ready = 1; aoe_comm_system_message(&s, &m, &o);
    assert(r.posts[0] == 0x17a9 && s.options_dirty == 1);

    /* create in a launched game is rejected; host in lobby takes slot 2 */
    memset(&r, 0, sizeof r); m.type = AOE_DPSYS_CREATE_PLAYER; m.player_id = 77;
    s.program_state = 5; aoe_comm_system_message(&s, &m, &o); assert(r.rejects == 1);
    s.program_state = 3; s.is_host = 1; s.max_players = 4; s.slot_taken[1] = 1;
    aoe_comm_system_message(&s, &m, &o);
    assert(r.assigned == 2 && s.dpid[2] == 77 && s.slot_taken[2] == 2);

    /* destroy: host relays and drops slot 2 */
    m.type = AOE_DPSYS_DESTROY_PLAYER; s.first_slot = 1; s.last_slot = 4;
    aoe_comm_system_message(&s, &m, &o);
    assert(s.slot_dropped[2] == 1 && r.drops == 1);

    /* client receives options with the launch flag set */
    memset(&r, 0, sizeof r); s.is_host = 0; s.program_state = 3; s.my_slot = 3;
    memset(blk, 0xab, sizeof blk); m.type = AOE_DPSYS_SET_PLAYER_DATA;
    m.data = blk; m.data_size = sizeof blk; s.launched = 1; s.options_stage = 4;
    aoe_comm_system_message(&s, &m, &o);
    assert(s.options[5] == 0xab && s.program_state == 5 && s.game_stage == 4);
    assert(s.my_serial == 6001 && s.expected_serial[2] == 4000);

    /* host message before game start: leave */
    memset(&r, 0, sizeof r); s.program_state = 3; m.type = AOE_DPSYS_HOST;
    aoe_comm_system_message(&s, &m, &o); assert(r.lost == 1 && s.is_host == 0);
    s.program_state = 5; s.game_stage = 6;
    aoe_comm_system_message(&s, &m, &o); assert(s.is_host == 1 && s.host_slot == 3);
    return 0;
}
