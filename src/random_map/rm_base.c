#include "random_map.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

uint32_t aoe_rm_float_bits(float f) {
    uint32_t u;
    memcpy(&u, &f, 4);
    return u;
}

float aoe_rm_bits_float(uint32_t bits) {
    float f;
    memcpy(&f, &bits, 4);
    return f;
}

int32_t aoe_rm_rand_scaled_ror(AoeRmModuleRor *m, int32_t n) {
    return (int32_t)aoe_rng_next(m->rng) * n / 0x7fff;
}

uint8_t *aoe_rm_tile_ror(const AoeRmModuleRor *m, int32_t x, int32_t y) {
    return &m->map->tiles[(size_t)y * (size_t)m->map->width + (size_t)x];
}

void aoe_rm_module_set_map_ror(AoeRmModuleRor *m, AoeRmMapRor *map) {
    AoeRmChildRor *c;
    if (!m) return;
    m->map = map;
    m->width = map ? map->width : 0;
    m->height = map ? map->height : 0;
    for (c = m->children; c; c = c->next) aoe_rm_module_set_map_ror(c->module, map);
}

int aoe_rm_module_alloc_grids_ror(AoeRmModuleRor *m) {
    int32_t x, y;
    size_t cells = (size_t)m->width * (size_t)m->height;
    m->grid = calloc(cells ? cells : 1, 1);
    m->grid_rows = calloc((size_t)(m->height > 0 ? m->height : 1), sizeof(*m->grid_rows));
    m->nodes = calloc(cells ? cells : 1, sizeof(*m->nodes));
    m->node_rows = calloc((size_t)(m->height > 0 ? m->height : 1), sizeof(*m->node_rows));
    if (!m->grid || !m->grid_rows || !m->nodes || !m->node_rows) {
        aoe_rm_module_free_grids_ror(m);
        return 0;
    }
    m->owns_grids = 1;
    for (y = 0; y < m->height; ++y) {
        m->grid_rows[y] = m->grid + (size_t)m->width * (size_t)y;
        m->node_rows[y] = m->nodes + (size_t)m->width * (size_t)y;
    }
    for (y = 0; y < m->height; ++y)
        for (x = 0; x < m->width; ++x) {
            m->node_rows[y][x].x = x;
            m->node_rows[y][x].y = y;
        }
    return 1;
}

void aoe_rm_module_free_grids_ror(AoeRmModuleRor *m) {
    if (!m) return;
    if (m->owns_grids) {
        free(m->grid);
        free(m->grid_rows);
        free(m->nodes);
        free(m->node_rows);
    }
    m->grid = NULL;
    m->grid_rows = NULL;
    m->nodes = NULL;
    m->node_rows = NULL;
    m->owns_grids = 0;
}

int aoe_rm_module_add_child_ror(AoeRmModuleRor *m, AoeRmModuleRor *child) {
    AoeRmChildRor *c;
    if (!m || !child) return 0;
    c = calloc(1, sizeof(*c));
    if (!c) return 0;
    c->next = m->children;
    c->module = child;
    m->children = c;
    return 1;
}

int aoe_rm_module_remove_child_ror(AoeRmModuleRor *m, AoeRmModuleRor *child) {
    AoeRmChildRor **slot;
    if (!m) return 0;
    for (slot = &m->children; *slot; slot = &(*slot)->next)
        if ((*slot)->module == child) {
            AoeRmChildRor *gone = *slot;
            *slot = gone->next;
            free(gone);
            return 1;
        }
    return 0;
}

void aoe_rm_module_init_ror(AoeRmModuleRor *m, const AoeRmModuleVtblRor *vt,
                            AoeRmMapRor *map, AoeRmModuleRor *parent,
                            int allocate, AoeRng *rng) {
    if (!m) return;
    memset(m, 0, sizeof(*m));
    m->vt = vt;
    m->parent = parent;
    m->rng = rng;
    m->map = map;
    if (map) {
        m->width = map->width;
        m->height = map->height;
    }
    if (parent) {
        aoe_rm_module_add_child_ror(parent, m);
        m->grid = parent->grid;
        m->grid_rows = parent->grid_rows;
        m->nodes = parent->nodes;
        m->node_rows = parent->node_rows;
        if (!m->rng) m->rng = parent->rng;
    } else if (allocate) {
        aoe_rm_module_alloc_grids_ror(m);
    }
}

void aoe_rm_module_release_ror(AoeRmModuleRor *m) {
    if (!m) return;
    while (m->children) {
        AoeRmModuleRor *child = m->children->module;
        if (!child) {
            aoe_rm_module_remove_child_ror(m, NULL);
            continue;
        }
        if (child->vt && child->vt->destroy) child->vt->destroy(child);
        else aoe_rm_module_release_ror(child);
        free(child);
    }
    if (!m->parent) {
        aoe_rm_module_free_grids_ror(m);
    } else {
        aoe_rm_module_remove_child_ror(m->parent, m);
        m->grid = NULL;
        m->grid_rows = NULL;
        m->nodes = NULL;
        m->node_rows = NULL;
    }
    m->map = NULL;
    m->width = m->height = 0;
}

float aoe_rm_module_priority_ror(const AoeRmModuleRor *m) {
    return m ? m->priority : 0.0f;
}

int aoe_rm_module_run_children_ror(AoeRmModuleRor *m) {
    AoeRmChildRor *sorted = NULL, *node, *next;
    if (!m) return 0;
    for (node = m->children; node; node = next) {
        float p = aoe_rm_module_priority_ror(node->module);
        next = node->next;
        if (!sorted || aoe_rm_module_priority_ror(sorted->module) > p) {
            node->next = sorted;
            sorted = node;
        } else {
            AoeRmChildRor *at = sorted;
            while (at->next && !(aoe_rm_module_priority_ror(at->next->module) > p))
                at = at->next;
            node->next = at->next;
            at->next = node;
        }
    }
    m->children = sorted;
    (void)aoe_rng_next(m->rng); /* "map start" log line */
    for (node = m->children; node; node = node->next) {
        if (!node->module->vt->run(node->module)) return 0;
        (void)aoe_rng_next(m->rng); /* "module complete" log line */
    }
    return 1;
}

void aoe_rm_list_init_ror(AoeRmNodeRor *head) {
    head->x = head->y = -1;
    head->next = head->prev = NULL;
    head->value = 0;
    head->priority = 0.0f;
}

void aoe_rm_list_detach_ror(AoeRmNodeRor *head) {
    if (head->next) {
        head->next->prev = NULL;
        head->next = NULL;
    }
}

void aoe_rm_node_unlink_ror(AoeRmNodeRor *node) {
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
    node->next = node->prev = NULL;
}

void aoe_rm_node_link_ror(AoeRmNodeRor *after, AoeRmNodeRor *node) {
    if (node->next || node->prev) aoe_rm_node_unlink_ror(node);
    node->prev = after;
    node->next = after->next;
    if (after->next) after->next->prev = node;
    after->next = node;
}

void aoe_rm_list_push_ror(AoeRmModuleRor *m, AoeRmNodeRor *head, int32_t x,
                          int32_t y, uint32_t value, float priority) {
    AoeRmNodeRor *node = &m->node_rows[y][x], *at;
    aoe_rm_node_unlink_ror(node);
    at = head->next;
    if (!at || priority <= at->priority) {
        aoe_rm_node_link_ror(head, node);
    } else {
        while (at->next && at->next->priority < priority) at = at->next;
        aoe_rm_node_link_ror(at, node);
    }
    node->value = value;
    node->priority = priority;
}

AoeRmNodeRor *aoe_rm_list_pop_ror(AoeRmNodeRor *head, int32_t *x, int32_t *y,
                                  uint32_t *value) {
    AoeRmNodeRor *node = head->next;
    if (!node) return NULL;
    *x = node->x;
    *y = node->y;
    *value = node->value;
    aoe_rm_node_unlink_ror(node);
    return node;
}

void aoe_rm_nodes_unlink_all_ror(AoeRmModuleRor *m) {
    int32_t x, y;
    for (y = 0; y < m->height; ++y)
        for (x = 0; x < m->width; ++x) {
            m->node_rows[y][x].next = NULL;
            m->node_rows[y][x].prev = NULL;
        }
}

void aoe_rm_list_shuffle_ror(AoeRmModuleRor *m, AoeRmNodeRor *head) {
    AoeRmNodeRor buckets[100];
    int pass, i;
    for (i = 0; i < 100; ++i) aoe_rm_list_init_ror(&buckets[i]);
    for (pass = 0; pass < 2; ++pass) {
        while (head->next) {
            int32_t b = aoe_rm_rand_scaled_ror(m, 100);
            /* rand() == 0x7fff gives 100, one past the original's bucket
               array (it then links into its own stack frame); the port
               uses the last bucket. */
            if (b > 99) b = 99;
            aoe_rm_node_link_ror(&buckets[b], head->next);
        }
        for (i = 0; i < 100; i += 2) {
            for (;;) {
                if (!buckets[i].next && !buckets[i + 1].next) break;
                if (buckets[i].next) aoe_rm_node_link_ror(head, buckets[i].next);
                if (buckets[i + 1].next) aoe_rm_node_link_ror(head, buckets[i + 1].next);
            }
        }
    }
}

/* One straight step of the search: the neighbour (nx, ny) with the
   distance estimate `estimate`. Returns 1 when the goal was reached, -1
   when the neighbour is open (queued now or earlier), 0 when blocked. */
static int straight(AoeRmModuleRor *m, AoeRmNodeRor *open, int32_t nx,
                    int32_t ny, uint8_t code, float cost_so_far,
                    double estimate, const float *costs, int32_t *goal_x,
                    int32_t *goal_y) {
    uint8_t *cell = &m->grid_rows[ny][nx];
    if (*cell == 0) {
        float c = costs[*aoe_rm_tile_ror(m, nx, ny) & 0x1f];
        float sum;
        if (c <= 0.0f) {
            *cell = 1;
            return 0;
        }
        sum = (float)((double)cost_so_far + (double)c);
        *cell = code;
        aoe_rm_list_push_ror(m, open, nx, ny, aoe_rm_float_bits(sum),
                             (float)((double)sum + estimate));
        return -1;
    }
    if (*cell == 2) {
        *cell = code;
        *goal_x = nx;
        *goal_y = ny;
        return 1;
    }
    return *cell != 1 ? -1 : 0;
}

/* A diagonal step, taken only when both straight steps beside it are
   open. */
static int diagonal(AoeRmModuleRor *m, AoeRmNodeRor *open, int32_t nx,
                    int32_t ny, uint8_t code, float cost_so_far, int ok,
                    double estimate, const float *costs, int32_t *goal_x,
                    int32_t *goal_y) {
    uint8_t *cell = &m->grid_rows[ny][nx];
    if (*cell == 0) {
        float c = costs[*aoe_rm_tile_ror(m, nx, ny) & 0x1f];
        float sum;
        if (c <= 0.0f) {
            *cell = 1;
            return 0;
        }
        if (!ok) return 0;
        sum = (float)((double)c * (double)1.36f + (double)cost_so_far);
        *cell = code;
        aoe_rm_list_push_ror(m, open, nx, ny, aoe_rm_float_bits(sum),
                             (float)(estimate + (double)sum));
        return 0;
    }
    if (*cell == 2) {
        *cell = code;
        *goal_x = nx;
        *goal_y = ny;
        return 1;
    }
    return 0;
}

static float distance(int32_t x, int32_t y, int32_t gx, int32_t gy) {
    int32_t dx = x - gx, dy = y - gy;
    return (float)sqrt((double)(dy * dy + dx * dx));
}

int aoe_rm_search_ror(AoeRmModuleRor *m, AoeRmNodeRor *open, int32_t x,
                      int32_t y, int32_t *goal_x, int32_t *goal_y,
                      const float *costs) {
    int32_t max_x, max_y;
    float cost = 0.0f, dist;
    if (!m || !open || !goal_x || !goal_y || !costs) return 0;
    max_x = m->width - 1;
    max_y = m->height - 1;
    aoe_rm_nodes_unlink_all_ror(m);
    memset(m->grid, 0, (size_t)m->width * (size_t)m->height);
    m->grid_rows[y][x] = 3;
    m->grid_rows[*goal_y][*goal_x] = 2;
    if (costs[*aoe_rm_tile_ror(m, *goal_x, *goal_y) & 0x1f] <= 0.0f) return 0;
    dist = distance(x, y, *goal_x, *goal_y);
    while (dist != 0.0f) {
        int down = 0, up = 0, left = 0, r;
        uint32_t bits;
        if (y < max_y) {
            double e = *goal_y > y ? (double)dist - 1.0 : (double)dist + 1.0;
            r = straight(m, open, x, y + 1, 7, cost, e, costs, goal_x, goal_y);
            if (r > 0) return 1;
            down = r != 0;
        }
        if (y > 0) {
            double e = *goal_y < y ? (double)dist - 1.0 : (double)dist + 1.0;
            r = straight(m, open, x, y - 1, 6, cost, e, costs, goal_x, goal_y);
            if (r > 0) return 1;
            up = r != 0;
        }
        if (x > 0) {
            float e = (float)(*goal_x < x ? (double)dist - 1.0 : (double)dist + 1.0);
            r = straight(m, open, x - 1, y, 4, cost, e, costs, goal_x, goal_y);
            if (r > 0) return 1;
            left = r != 0;
            if (y > 0 &&
                diagonal(m, open, x - 1, y - 1, 8, cost, up && left,
                         *goal_y < y ? (double)e - (double)0.41f
                                     : (double)e + (double)0.41f,
                         costs, goal_x, goal_y))
                return 1;
            if (y < max_y &&
                diagonal(m, open, x - 1, y + 1, 9, cost, down && left,
                         *goal_y > y ? (double)e - (double)0.41f
                                     : (double)e + (double)0.41f,
                         costs, goal_x, goal_y))
                return 1;
        }
        if (x < max_x) {
            float e = (float)(*goal_x > x ? (double)dist - 1.0 : (double)dist + 1.0);
            int right;
            r = straight(m, open, x + 1, y, 5, cost, e, costs, goal_x, goal_y);
            if (r > 0) return 1;
            right = r != 0;
            if (y > 0 &&
                diagonal(m, open, x + 1, y - 1, 10, cost, up && right,
                         *goal_y < y ? (double)e - (double)0.4f
                                     : (double)e + (double)0.4f,
                         costs, goal_x, goal_y))
                return 1;
            if (y < max_y &&
                diagonal(m, open, x + 1, y + 1, 11, cost, down && right,
                         *goal_y > y ? (double)e - (double)0.4f
                                     : (double)e + (double)0.4f,
                         costs, goal_x, goal_y))
                return 1;
        }
        if (!aoe_rm_list_pop_ror(open, &x, &y, &bits)) return 0;
        cost = aoe_rm_bits_float(bits);
        dist = distance(x, y, *goal_x, *goal_y);
    }
    if (open->next) open->next->prev = NULL;
    return 1;
}

int aoe_rm_find_path_ror(AoeRmModuleRor *m, int32_t x, int32_t y,
                         int32_t *goal_x, int32_t *goal_y,
                         const float *costs) {
    AoeRmNodeRor open;
    int found;
    aoe_rm_list_init_ror(&open);
    found = aoe_rm_search_ror(m, &open, x, y, goal_x, goal_y, costs);
    aoe_rm_list_detach_ror(&open);
    return found;
}
