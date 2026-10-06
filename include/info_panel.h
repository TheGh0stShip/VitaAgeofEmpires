#ifndef VITA_AOE_INFO_PANEL_H
#define VITA_AOE_INFO_PANEL_H

#include <stdint.h>

#include "ui_panel.h"

/* Patched Rise of Rome 1.0a game-screen object panel (tpnl_obj, vtable
   0x00549e7c, 0x004f83d0..0x004fa150): the viewed player's selected object
   (player +0x134) with its picture, owner, name, hit points and up to six
   stat rows, or, in score mode, every player's score. Its idle routine
   redraws when a snapshot of what it shows goes stale and tells the parent
   (action 1) when the object changed enough to need new commands. */

/* What the panel reads from the selected object, by original offset.
   `owner` and `template_id` are identities only: the panel compares them
   and never dereferences them. */
typedef struct {
    const void *owner;          /* +0x0c */
    int32_t id;                 /* +0x04 */
    const void *template_id;    /* +0x08 */
    uint8_t type;               /* template +0x04: 0x50 building; above
                                   0x45 (but not 0x5a) a combat type */
    int16_t object_class;       /* template +0x10 */
    int16_t name_id;            /* template +0x0c */
    const char *name;           /* template +0x08 */
    int16_t max_hp;             /* template +0x26 */
    float template_28;          /* template +0x28 */
    uint8_t capacity;           /* template +0x2c */
    int16_t icon;               /* template +0x4c, -1 none */
    uint8_t panel_kind;         /* template +0x8f */
    int16_t building_icon_base; /* template +0x16e (kind 2) */
    uint8_t icon_set;           /* +0x1dc: the building picture set */
    uint8_t field_37;           /* +0x37 */
    float hp;                   /* +0x30 */
    float x, y;                 /* +0x38, +0x3c */
    float amount;               /* +0x44 */
    uint8_t object_state;       /* +0x48: 0 under construction */
    int16_t resource_type;      /* +0x4c */
    const void *contents_first; /* +0x24 -> +0x04 */
    int16_t contents_count;     /* +0x24 -> +0x08 */
    uint8_t has_action;         /* 0x004065f0: the action list is not empty */
    /* Virtuals +0x230 into `attack`, +0x234 into `armor`, +0x248 into
       `pierce`: the value and the shown base; +0x238 (range) and +0x23c
       floats. The names follow the slots: the type 70 / 80 classes
       answer +0x230 with the melee armour (0x004aee00, class 4 of the
       armour list) and +0x234 with the attack (0x00426a80, the largest
       attack amount), and the draw shows `armor` with item icon 7 and
       `attack` with icon 8 accordingly. */
    int16_t attack, attack_base;
    int16_t armor, armor_base;
    int16_t pierce, pierce_base;
    float range, range_base;
    float stat_23c, stat_23c_base;
    /* 0x004ad910: build percent (100 when built or with no build time). */
    int16_t build_percent;
    /* 0x004ad870: the head action's type (0x66 train, 0x67 research, else
       idle), its object/tech ID, progress and label. 0x004ae210: with
       no such action, the first queued unit (`queued`, `queued_id`,
       `queued_count`). */
    int16_t action_type, action_id, action_progress;
    char action_text[100];
    int queued;
    int16_t queued_id, queued_count;
    /* +0x184 head action (class 0x32) and its +0x2c. */
    int has_head_action;
    float head_action_2c;
    /* Owner: colour record (+0x10c) present, its +0x26 colour; its +0x4a
       player number and +0x109 civilisation. */
    int owner_has_color;
    int16_t owner_color;
    int16_t owner_number;
    uint8_t owner_civ;
    float owner_attribute_9;    /* owner +0x50[9] (a dock's stock) */
} AoeInfoObjectRor;

/* What the editor panel reads from a unit template, by original offset. */
typedef struct AoeInfoTemplateRor {
    int16_t icon;               /* +0x4c, -1 none */
    uint8_t panel_kind;         /* +0x8f */
    int16_t building_icon_base; /* +0x16e (kind 2) */
    void *sprite;               /* +0x18, drawn when there is no icon */
    int16_t name_id;            /* +0x0c */
    const char *name;           /* +0x08 */
    int16_t max_hp;             /* +0x26 */
    int16_t displayed_13c;      /* +0x13c: item icon 8 */
    int16_t displayed_13e;      /* +0x13e: item icon 7 */
    float range_114;            /* +0x114 (kind 2) */
    float range_140;            /* +0x140 (kinds 4 and 6) */
    int16_t resource_type[3];   /* +0x6a */
    float resource_amount[3];   /* +0x70 */
    /* 0x004a4b90 (aoe_combat_panel_pierce_armor_ror): a type 70
       template's last class 3 armor, else 0. */
    int16_t pierce;
} AoeInfoTemplateRor;

typedef struct {
    void *context;
    /* The player's selected object (+0x134), NULL for none or no player. */
    void *(*selected)(void *context, const void *player);
    int (*read)(void *context, void *object, AoeInfoObjectRor *out);
    /* A player's attribute (+0x50[index]) and civilisation (+0x109). */
    float (*attribute)(void *context, const void *player, int index);
    uint8_t (*civilization)(void *context, const void *player);
    /* 0x00451370 on the player's object list: the nearest object of the
       class in `state` near (x, y) in the unit's area. */
    int (*nearest)(void *context, const void *player, int16_t object_class,
                   float x, float y, uint8_t state, void *unit, float *ox,
                   float *oy);
    /* Names: multiplayer (game +0x9a0) and the comm name of a player
       index; game vtable +0x28(0x69, civ) civilisation name; the string
       table; the game's +0xc20 ID display. */
    int (*multiplayer)(void *context);
    const char *(*comm_name)(void *context, int32_t index);
    void (*civ_name)(void *context, uint8_t civ, char *out, int size);
    void (*string)(void *context, int32_t id, char *out, int size);
    int (*show_ids)(void *context);
    /* Score mode: slot i's player number (0x0041c080), the world player
       count (+0x3c), a player's colour (+0x10c +0x26), score (+0x34
       0x00454170) and slot i's name (0x004241a0). */
    int32_t (*slot_player)(void *context, int32_t slot);
    int32_t (*player_count)(void *context);
    int16_t (*player_color)(void *context, int32_t player);
    int32_t (*player_score)(void *context, int32_t player);
    const char *(*slot_name)(void *context, int32_t slot);
    /* Shapes: draw a frame (remap >= 0: player colours from that palette
       index), frame count (0x0049fa90), load and free. */
    void (*draw_shape)(void *context, AoeUiAreaRor *area, void *shape,
                       int32_t frame, int32_t x, int32_t y, int32_t remap);
    int32_t (*shape_frames)(void *context, void *shape);
    void *(*load_shape)(void *context, const char *file, int32_t id);
    void (*free_shape)(void *context, void *shape);
    /* The scenario editor's template panel: read a template, a player's
       number (+0x4a) and colour record (+0x10c, its +0x26 index), and a
       template sprite's (+0x18) bounds (patched 0x4a5c60, untranslated) and draw (0x004a55e0,
       in the player's colours). */
    int (*read_template)(void *context, const void *tmpl,
                         struct AoeInfoTemplateRor *out);
    int16_t (*player_number)(void *context, const void *player);
    int (*player_colour)(void *context, const void *player, int16_t *index);
    int (*sprite_bounds)(void *context, void *sprite, int32_t *min_x,
                         int32_t *min_y);
    void (*draw_sprite)(void *context, AoeUiAreaRor *area, void *sprite,
                        int32_t x, int32_t y, const void *player);
} AoeInfoPanelOpsRor;

typedef struct {
    AoeUiPanelRor panel;
    uintptr_t font;            /* +0xf4 */
    int32_t font_w, font_h;    /* +0xf8, +0xfc */
    void *health;              /* +0x100 "health.shp" 50745 */
    void *item_icons;          /* +0x104 "itemicon.shp" 50731 */
    void *unit_icons;          /* +0x108 */
    void *const *building_icons; /* +0x10c, by icon set */
    const void *player;        /* +0x110 */
    void *object;              /* +0x114 */
    uint32_t last_ms;          /* +0x118 */
    uint32_t interval_ms;      /* +0x11c, 500 */
    uint8_t mode;              /* +0x120: 1 object, 2 scores */
    /* The snapshot (0x004f9ee0). */
    const void *snap_template; /* +0x124 */
    int16_t snap_resource;     /* +0x12c, -1 */
    float snap_amount;         /* +0x130 */
    uint8_t snap_state;        /* +0x134 */
    float snap_percent;        /* +0x138 */
    const void *snap_contents; /* +0x13c */
    int16_t snap_count;        /* +0x140 */
    float snap_hp;             /* +0x14c */
    int16_t snap_attack, snap_152, snap_armor; /* +0x150, +0x152, +0x154 */
    float snap_range, snap_23c;                /* +0x158, +0x15c */
    int16_t snap_action, snap_action_id, snap_progress; /* +0x160.. */
    char snap_text[100];       /* +0x166 */
    float snap_template_28;    /* +0x1d0 */
    int32_t snap_pop, snap_pop_cap; /* +0x1d4, +0x1d8 */
    uint8_t snap_37;           /* +0x1dc */
    const void *snap_owner;    /* +0x1e0 */
    uint8_t snap_has_action;   /* +0x1e8 */
    int32_t scores[9];         /* +0x1ec, by player number */
    int32_t snap_head;         /* +0x210 */
    const AoeInfoPanelOpsRor *ops;
} AoeInfoPanelRor;

const AoeUiPanelVtblRor *aoe_info_panel_vtable_ror(void);

/* 0x004f83d0: setup (area, parent, font), the unit and building
   pictures, the player, then health.shp and itemicon.shp; 0 when setup
   fails. 0x004f8650 destructor (deleting 0x004f8630); a derived panel
   (vtable 0x00547cc0, the scenario editor's) has the same destructor
   0x004a4310 (deleting 0x004a42f0); its constructor 0x004a42a0 runs
   this setup and clears +0x214 / +0x218, and 0x004a4320(value, object)
   stores +0x214, the shown object (+0x110) and +0x218 = the object's
   +0x54 byte (0 without an object). */
int aoe_info_panel_init_ror(AoeInfoPanelRor *i, AoeUiSystemRor *system,
                            const AoeInfoPanelOpsRor *ops, AoeUiAreaRor *area,
                            AoeUiPanelRor *parent, uintptr_t font,
                            int32_t font_w, int32_t font_h, void *unit_icons,
                            void *const *building_icons, const void *player);
/* 0x004f86f0 the player (and its selection); 0x004f8720 the object
   (snapshot, redraw); 0x004f9ee0 the snapshot. */
void aoe_info_panel_set_player_ror(AoeInfoPanelRor *i, const void *player);
void aoe_info_panel_set_object_ror(AoeInfoPanelRor *i, void *object);
void aoe_info_panel_snapshot_ror(AoeInfoPanelRor *i);
/* +0xe4 0x004f97b0: a stat row: the item icon (unless -1) at row * 20,
   then by kind 0 "%d" a, 1 "%d+%d" a and b - a (just a unless b > a), 2
   "%d/%d", 3 "%d%%" in white. */
void aoe_info_panel_item_ror(AoeInfoPanelRor *i, int32_t row, int32_t icon,
                             int32_t kind, int32_t a, int32_t b);
/* 0x004f9530 the score view: each slot's player with a colour by its
   colour index, its name, and its score right-aligned, 14 pixels apart. */
void aoe_info_panel_draw_scores_ror(AoeInfoPanelRor *i);
/* +0x30 0x004f8740 draw; +0x4c 0x004f9970 idle. */
void aoe_info_panel_draw_ror(AoeUiPanelRor *p);
int32_t aoe_info_panel_idle_ror(AoeUiPanelRor *p);

/* The scenario editor's template panel (vtable 0x00547cc0): the object
   panel showing a unit template for a player (+0x110). */
typedef struct {
    AoeInfoPanelRor base;
    const void *tmpl;          /* +0x214 */
    uint8_t icon_set;          /* +0x218 */
} AoeInfoEditorPanelRor;

/* +0xe4 0x004a49a0: the editor's stat row: the item icon (unless -1) at
   (x + 0x3c, y + 0x1e + row * 18), the text at (x + 100, y + 0x20 +
   row * 18); kinds as aoe_info_panel_item_ror. */
void aoe_info_editor_item_ror(AoeInfoEditorPanelRor *e, int32_t row,
                              int32_t icon, int32_t kind, int32_t a, int32_t b);
/* +0x30 0x004a4350: the editor's draw. */
void aoe_info_editor_draw_ror(AoeInfoEditorPanelRor *e);

#endif
