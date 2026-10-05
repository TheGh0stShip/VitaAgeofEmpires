/* Patched RoR 1.0a random-map module base and search 0x0046bd00..0x0046c4c0. */
#include "random_map.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static int order[8], n_order;
static int run_a(AoeRmModuleRor *m) { (void)m; order[n_order++] = 1; return 1; }
static int run_b(AoeRmModuleRor *m) { (void)m; order[n_order++] = 2; return 1; }
static int run_c(AoeRmModuleRor *m) { (void)m; order[n_order++] = 3; return 0; }
static const AoeRmModuleVtblRor va = {run_a, NULL}, vb = {run_b, NULL}, vc = {run_c, NULL};

int main(void) {
    AoeRng rng, ref;
    uint8_t tiles[8 * 6];
    AoeRmMapRor map = {8, 6, tiles, NULL, NULL, NULL};
    AoeRmModuleRor root;
    AoeRmModuleRor *a = calloc(1, sizeof *a), *b = calloc(1, sizeof *b), *c = calloc(1, sizeof *c);
    float costs[32] = {0};
    int32_t gx, gy, i;
    AoeRmNodeRor head;

    aoe_rng_seed(&rng, 7);
    memset(tiles, 0, sizeof tiles);          /* terrain 0 everywhere */
    aoe_rm_module_init_ror(&root, NULL, &map, NULL, 1, &rng);
    assert(root.grid && root.node_rows[5][7].x == 7 && root.node_rows[5][7].y == 5);

    /* Children share the grids; they run in ascending priority. */
    aoe_rm_module_init_ror(a, &va, &map, &root, 0, NULL);
    aoe_rm_module_init_ror(b, &vb, &map, &root, 0, NULL);
    a->priority = 1.5f;
    b->priority = 1.0f;
    assert(a->grid == root.grid && a->rng == &rng);
    ref = rng;
    assert(aoe_rm_module_run_children_ror(&root));
    assert(n_order == 2 && order[0] == 2 && order[1] == 1);
    /* One rand() for the start line and one after each module. */
    aoe_rng_next(&ref); aoe_rng_next(&ref); aoe_rng_next(&ref);
    assert(rng.state == ref.state);
    /* A failing module stops the run. */
    aoe_rm_module_init_ror(c, &vc, &map, &root, 0, NULL);
    c->priority = 0.5f;
    n_order = 0;
    assert(!aoe_rm_module_run_children_ror(&root) && n_order == 1 && order[0] == 3);

    /* Priority list: ascending; a new entry goes before equal ones. */
    aoe_rm_list_init_ror(&head);
    aoe_rm_list_push_ror(&root, &head, 1, 1, 10, 5.0f);
    aoe_rm_list_push_ror(&root, &head, 2, 1, 11, 2.0f);
    aoe_rm_list_push_ror(&root, &head, 3, 1, 12, 5.0f);
    aoe_rm_list_push_ror(&root, &head, 4, 1, 13, 9.0f);
    {
        int32_t x, y; uint32_t v;
        assert(aoe_rm_list_pop_ror(&head, &x, &y, &v) && x == 2 && v == 11);
        assert(aoe_rm_list_pop_ror(&head, &x, &y, &v) && x == 3);
        assert(aoe_rm_list_pop_ror(&head, &x, &y, &v) && x == 1);
        assert(aoe_rm_list_pop_ror(&head, &x, &y, &v) && x == 4);
        assert(!aoe_rm_list_pop_ror(&head, &x, &y, &v));
    }

    /* Shuffle keeps every node and draws one rand() per node per pass. */
    aoe_rm_nodes_unlink_all_ror(&root);
    for (i = 0; i < 6; ++i) aoe_rm_list_push_ror(&root, &head, i, 0, 0, (float)i);
    ref = rng;
    aoe_rm_list_shuffle_ror(&root, &head);
    for (i = 0; i < 12; ++i) aoe_rng_next(&ref);
    assert(rng.state == ref.state);
    {
        AoeRmNodeRor *n;
        int seen = 0;
        for (n = head.next; n; n = n->next) seen |= 1 << n->x;
        assert(seen == 0x3f);
    }
    aoe_rm_list_detach_ror(&head);

    /* Search: a wall of terrain 1 (cost 0) with a gap at y = 5. */
    costs[0] = 1.0f;
    for (i = 0; i < 5; ++i) tiles[i * 8 + 4] = 1;
    gx = 7; gy = 0;
    assert(aoe_rm_find_path_ror(&root, 0, 0, &gx, &gy, costs));
    assert(gx == 7 && gy == 0);
    assert(root.grid_rows[0][0] == 3 && root.grid_rows[0][4] == 1);
    /* The goal cell holds the step that reached it. */
    assert(root.grid_rows[0][7] >= 4 && root.grid_rows[0][7] <= 11);
    /* Through the gap only: (4, 5) was reached. */
    assert(root.grid_rows[5][4] >= 4);
    /* No way through. */
    tiles[5 * 8 + 4] = 1;
    gx = 7; gy = 0;
    assert(!aoe_rm_find_path_ror(&root, 0, 0, &gx, &gy, costs));
    /* A goal on blocked terrain fails at once. */
    gx = 4; gy = 2;
    assert(!aoe_rm_find_path_ror(&root, 0, 0, &gx, &gy, costs));
    /* Start on the goal. */
    gx = 1; gy = 1;
    assert(aoe_rm_find_path_ror(&root, 1, 1, &gx, &gy, costs));

    aoe_rm_module_release_ror(&root);
    assert(!root.children && !root.grid);
    return 0;
}
