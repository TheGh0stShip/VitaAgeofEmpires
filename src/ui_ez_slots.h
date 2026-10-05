#ifndef VITA_AOE_UI_EZ_SLOTS_H
#define VITA_AOE_UI_EZ_SLOTS_H

/* Internal: the easy panel's slot functions and vtable layout, shared
   with its subclasses (dialogs and screens). */
#include "ui_ez.h"

void aoe_ui_ez_destroy_slot_ror(AoeUiPanelRor *p);
void aoe_ui_ez_begin_draw_slot_ror(AoeUiPanelRor *p, int32_t clear);
void aoe_ui_ez_draw_slot_ror(AoeUiPanelRor *p);
int32_t aoe_ui_ez_mouse_down_slot_ror(AoeUiPanelRor *p, uint8_t button,
                                      int32_t x, int32_t y, int32_t c,
                                      int32_t s);
int32_t aoe_ui_ez_action_slot_ror(AoeUiPanelRor *p, AoeUiPanelRor *from,
                                  int32_t code, int32_t value, int32_t extra);
void aoe_ui_ez_set_focus_slot_ror(AoeUiPanelRor *p, int32_t focus);
/* The screen subclass's fading focus (0x004681c0). */
void aoe_ui_screen_focus_slot_ror(AoeUiPanelRor *p, int32_t focus);
int32_t aoe_ui_ez_no_key_action_ror(AoeUiPanelRor *p, int32_t a, int32_t b,
                                    int32_t c, int32_t d, int32_t e);
void aoe_ui_ez_draw_background_slot_ror(AoeUiEzRor *ez, int32_t parent_rect);
void aoe_ui_ez_set_reference_slot_ror(AoeUiEzRor *ez, int32_t w, int32_t h);

#define AOE_UI_EZ_SLOTS_M(destroy_fn, idle_fn, paint_fn, key_fn, help_fn, action_fn, focus_fn, mouse_fn) \
    AOE_UI_EZ_SLOTS_MD(destroy_fn, aoe_ui_ez_draw_slot_ror, idle_fn, paint_fn, key_fn, help_fn, action_fn, focus_fn, mouse_fn)

#define AOE_UI_EZ_SLOTS_MD(destroy_fn, draw_fn, idle_fn, paint_fn, key_fn, help_fn, action_fn, focus_fn, mouse_fn) \
    AOE_UI_EZ_SLOTS_MDU(destroy_fn, draw_fn, idle_fn, paint_fn, key_fn, help_fn, action_fn, focus_fn, mouse_fn, aoe_ui_panel_ignore_ror)

#define AOE_UI_EZ_SLOTS_MDU(destroy_fn, draw_fn, idle_fn, paint_fn, key_fn, help_fn, action_fn, focus_fn, mouse_fn, user_fn) \
    {destroy_fn,                                                              \
     aoe_ui_panel_setup_ror,                                                  \
     aoe_ui_panel_set_rect_ltrb_ror,                                          \
     aoe_ui_panel_set_rect_ror,                                               \
     aoe_ui_panel_set_color_ror,                                              \
     aoe_ui_panel_set_active_ror,                                             \
     aoe_ui_panel_set_position_mode_ror,                                      \
     aoe_ui_panel_set_fixed_position_ror,                                     \
     aoe_ui_panel_set_redraw_ror,                                             \
     aoe_ui_panel_redraw_overlaps_ror,                                        \
     aoe_ui_ez_begin_draw_slot_ror,                                                           \
     aoe_ui_panel_end_draw_ror,                                               \
     draw_fn,                                                                 \
     aoe_ui_panel_draw_rect_ror,                                              \
     aoe_ui_panel_draw_offset_ror,                                            \
     aoe_ui_panel_draw_parent_rect_ror,                                       \
     aoe_ui_panel_draw_parent_offset_ror,                                     \
     aoe_ui_panel_validate_ror,                                               \
     aoe_ui_panel_handle_message_ror,                                         \
     idle_fn,                                            \
     aoe_ui_panel_handle_size_ror,                                            \
     paint_fn,                                                                \
     aoe_ui_panel_key_down_ror,                                               \
     aoe_ui_panel_char_ror,                                                   \
     aoe_ui_panel_ignore_ror,                                                 \
     user_fn,                                                                 \
     aoe_ui_panel_ignore_ror,                                                 \
     aoe_ui_panel_ignore_ror,                                                 \
     mouse_fn,                                                           \
     aoe_ui_panel_mouse_move_ror,                                             \
     aoe_ui_panel_mouse_up_ror,                                               \
     aoe_ui_panel_mouse_dbl_ror,                                              \
     {aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,                 \
      aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,                 \
      aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,                 \
      aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,                 \
      aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,                 \
      aoe_ui_panel_no_action_ror},                                            \
     key_fn,                                                           \
     help_fn,                                                                 \
     aoe_ui_panel_no_char_action_ror,                                         \
     action_fn,                                                               \
     aoe_ui_panel_get_true_rect_ror,                                          \
     aoe_ui_panel_is_inside_ror,                                              \
     focus_fn,                                                            \
     aoe_ui_panel_set_tab_order_ror,                                          \
     aoe_ui_panel_get_help_ror,                                               \
     aoe_ui_panel_stop_sound_ror,                                             \
     aoe_ui_panel_restart_sound_ror,                                          \
     aoe_ui_panel_slot_e0_ror}

#define AOE_UI_EZ_SLOTS(destroy_fn, idle_fn, paint_fn, key_fn, help_fn,      \
                        action_fn, focus_fn)                                  \
    AOE_UI_EZ_SLOTS_M(destroy_fn, idle_fn, paint_fn, key_fn, help_fn,        \
                      action_fn, focus_fn, aoe_ui_ez_mouse_down_slot_ror)

#define AOE_UI_EZ_DEFAULT_SLOTS(destroy_fn, paint_fn, help_fn)               \
    AOE_UI_EZ_SLOTS(destroy_fn, aoe_ui_panel_handle_idle_ror, paint_fn,      \
                    aoe_ui_ez_no_key_action_ror, help_fn,                     \
                    aoe_ui_ez_action_slot_ror, aoe_ui_ez_set_focus_slot_ror)

#endif
