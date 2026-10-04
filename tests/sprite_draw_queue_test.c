#include "sprite_draw.h"
#include "sprite_instance.h"

#include <assert.h>
#include <string.h>

/* The view sprite list (RoR 1.0a disc 0x00518d90 / 0x00515844 /
   0x0051606f): parts queued during the tile walk carry the graphic layer
   and the live draw's row key, rows outside [-200, height + 249] are not
   queued, and the flush order is layer, then row, then queue order. */

static void enqueue(AoeSpriteDrawQueue *q, AoeSpriteIndexSurface *s,
                    const DatGraphic *g, int32_t row) {
    static DrsArchive dummy;
    DrsArchive *archives[1] = {&dummy};
    AoeSpriteInstance instance;
    memset(&instance, 0, sizeof(instance));
    assert(aoe_sprite_instance_init(&instance, g, NULL, 0, 0));
    q->row_key = row;
    assert(aoe_sprite_draw_instance_bounds_cached_indexed_ror(
               &instance, 0, 10, 10, 20, 20, archives, 1, NULL, s, 3, NULL,
               NULL, NULL) == AOE_GRAPHIC_ART_OK);
}

int main(void) {
    uint8_t pixels[4];
    AoeSpriteDrawQueue q;
    AoeSpriteIndexSurface s = {pixels, sizeof(pixels), 2, 2, 2, &q};
    DatGraphic low = {.present = 1, .frame_count = 1, .angle_count = 1,
                      .layer = 5, .player_color = -1};
    DatGraphic high = low;
    high.layer = 20;
    memset(&q, 0, sizeof(q));
    aoe_sprite_draw_queue_reset(&q, -200, 2 + 249);

    enqueue(&q, &s, &high, 10); /* seq 0 */
    enqueue(&q, &s, &low, 40);  /* seq 1 */
    enqueue(&q, &s, &low, 10);  /* seq 2 */
    enqueue(&q, &s, &low, 10);  /* seq 3: same row, later */
    enqueue(&q, &s, &low, -201); /* outside the list rows: dropped */
    enqueue(&q, &s, &low, 252);  /* outside (height 2 + 249): dropped */
    assert(q.count == 4);
    assert(q.entries[0].layer == 20 && q.entries[0].row == 10);
    assert(q.entries[1].player_color == 3);

    aoe_sprite_draw_queue_sort(&q);
    /* Layer 5 first (rows 10, 10, 40), the layer-20 part last. */
    assert(q.entries[0].layer == 5 && q.entries[0].row == 10 &&
           q.entries[0].sequence == 2);
    assert(q.entries[1].row == 10 && q.entries[1].sequence == 3);
    assert(q.entries[2].row == 40 && q.entries[2].sequence == 1);
    assert(q.entries[3].layer == 20 && q.entries[3].sequence == 0);

    aoe_sprite_draw_queue_reset(&q, -200, 251);
    assert(q.count == 0 && q.next_sequence == 0);
    aoe_sprite_draw_queue_free(&q);
    return 0;
}
