#include "net_misc.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int opens, cleanups;
static int fail_until = 3;
static void *lopen(void *c, const char *p) { (void)c; (void)p; return ++opens >= fail_until ? (void *)1 : NULL; }
static void lclose(void *c, void *f) { (void)c; (void)f; }
static uint32_t now(void *c) { (void)c; return 5000; }
static int ghn(void *c, char *o, int n) { (void)c; snprintf(o, (size_t)n, "box"); return 0; }
static const char *const al[] = {"a", "bb", NULL};
static const uint8_t ip1[4] = {10, 0, 0, 7}, ip2[4] = {192, 168, 1, 2};
static const uint8_t (*const ips[])[4] = {&ip1, &ip2, NULL};
static int lookup(void *c, const char *n, const char *const **a, const uint8_t (*const **b)[4]) {
    (void)c; if (strcmp(n, "box") != 0) return 0; *a = al; *b = ips; return 1;
}
static void cleanup(void *c) { (void)c; cleanups++; }
static uint32_t pw, pm, pwp, plp;
static void post(void *c, uint32_t w, uint32_t m, uint32_t a, uint32_t b) { (void)c; pw = w; pm = m; pwp = a; plp = b; }

int main(void) {
    AoeDebugLogRor l;
    AoeDebugLogOpsRor lops = {NULL, lopen, lclose, now};
    AoeHostInfoRor h;
    AoeHostOpsRor hops = {NULL, ghn, lookup, cleanup};
    AoeWindowPosterRor p;
    char out[64];
    memset(&l, 0, sizeof l);
    assert(!aoe_debug_log_open_ror(&l, &lops) && opens == 0);
    l.enabled = 1;
    assert(aoe_debug_log_open_ror(&l, &lops) && opens == 3 && strcmp(l.path, "C:\\AOELOG02.txt") == 0);
    assert(l.open && l.start_ms == 5000);
    aoe_debug_log_close_ror(&l, &lops);
    assert(!l.open && l.file == NULL);
    fail_until = 1000;
    opens = 0;
    assert(!aoe_debug_log_open_ror(&l, &lops) && opens == 25 && l.failed);
    aoe_debug_log_sequence_ror(&l, 1);
    assert(l.sequence == 1 && l.counter == 0);
    aoe_debug_log_flush_ror(&l, 5);
    assert(l.flush == 1);

    memset(&h, 0, sizeof h);
    assert(strcmp(aoe_host_name_ror(&h, &hops), "box") == 0);
    assert(aoe_host_aliases_ror(&h, &hops, out) == 2 && strcmp(aoe_host_alias_ror(&h, 1), "bb") == 0);
    assert(aoe_host_addresses_ror(&h, &hops, out, sizeof out) == 2);
    assert(strcmp(out, "10.0.0.7192.168.1.2") == 0 && strcmp(h.entries[1].address, "192.168.1.2") == 0);
    assert(strcmp(aoe_host_alias_ror(&h, 5), "") == 0);
    h.started = 1;
    aoe_host_release_ror(&h, &hops);
    assert(cleanups == 1 && h.name[0] == 0);
    aoe_poster_init_ror(&p, 77);
    aoe_poster_post_ror(&p, post, NULL, 0x17a4, 3);
    assert(pw == 77 && pm == 0x400 && pwp == 0x17a4 && plp == 3);
    puts("net_misc_test: ok");
    return 0;
}
