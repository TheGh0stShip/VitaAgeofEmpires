#ifndef VITA_AOE_ACHIEVEMENTS_PAGE_H
#define VITA_AOE_ACHIEVEMENTS_PAGE_H

#include <stdint.h>

#include "player_outcome.h"

/* Patched Rise of Rome 1.0a "Achievements Screen" pages (0x00478200,
   the screen's page switch): the page title, its column headings and,
   for each player row, the cells read from the player's outcome manager
   (player +0x34). Pages: 0 summary, 1 military, 2 economy, 3 religion,
   4 technology, 5 the scenario's text, 6 the timeline. The UI side (which
   panels show, the focus and the per-row visibility of the first
   `columns` cells) stays with the screen. */
enum {
    AOE_ACH_CELL_EMPTY,   /* "" */
    AOE_ACH_CELL_NUMBER,  /* itoa base 10 */
    AOE_ACH_CELL_PERCENT, /* "%d%%" */
    AOE_ACH_CELL_YES      /* string 4003 */
};

typedef struct {
    int kind;
    int32_t value;
} AoeAchievementsCellRor;

typedef struct {
    int32_t title_id;     /* +0x488 */
    int32_t columns;      /* +0x480 */
    int32_t heading[7];   /* +0x498..+0x4b0 (0 for none set) */
} AoeAchievementsPageRor;

/* The page's title and headings; 0 for an unknown page. */
int aoe_achievements_page_ror(int32_t page, AoeAchievementsPageRor *out);

/* One player row for pages 0..4 (0 otherwise): sums 0x0050ef70 by
   category, last event points 0x0050efa0 (a positive one shows 4003),
   samples 0x0050efc0, the manager's score +0x1c and, on page 1, the
   player's attribute 19 less attribute 11 (truncated). */
int aoe_achievements_row_ror(int32_t page, const AoeOutcomeManager *m,
                             const float *attributes, AoeAchievementsCellRor cells[7]);

/* A cell as the original shows it ("" / the number / the percentage /
   `yes` for string 4003). */
void aoe_achievements_cell_text_ror(const AoeAchievementsCellRor *c, const char *yes,
                                    char *out, int size);

#endif
