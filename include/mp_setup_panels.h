#ifndef VITA_AOE_MP_SETUP_PANELS_H
#define VITA_AOE_MP_SETUP_PANELS_H

#include "game_ui.h"

/* The panel side of the "MP Setup Screen" state's ops: the callbacks of
   AoeMpSetupScreenOpsRor that only touch the screen's own panels (the
   player-count dropdown, the colour buttons, the chat text, the tab order
   and focus, and the generic panel / button / list / input accessors the
   action handler uses) over an AoeMpSetupUiRor. The game and comm side
   (settings flags, the comm, dialogs, screens, the chat log) stay the
   director's: those fields are left as the caller set them. */

/* Store `u` as ops->context and fill only the panel callbacks:
   count_clear / count_append / count_index / count_select;
   color_text / color_state / color_rgb / color_count;
   chat_count / chat_bottom / chat_remove_first / chat_add /
   chat_scroll_bottom (chat_line is the chat log's 0x0041c9f0, game side,
   and is not set here); tab_order / focus; panel_active; button_state /
   button_value / button_set_state; list_id / list_selected / list_index /
   list_select; input_text / input_set. Every other field is untouched. */
void aoe_mp_setup_panels_bind_ror(AoeMpSetupScreenOpsRor *ops, AoeMpSetupUiRor *u);

/* The screen's vtable (0x00546d7c) handlers over an AoeMpSetupUiRor
   (defined in mp_setup_screen.c, bound in aoe_mp_setup_ui_vtable_ror).
   +0x4c 0x004894e0: the state's idle (aoe_mp_setup_screen_idle_ror), the
   base being the panel's idle 0x00453360 rather than the base_idle op.
   +0xb8 0x004895b0: the state's action handler
   (aoe_mp_setup_screen_action_ror) on the panel that fired; what it does
   not handle goes to the easy panel's 0x00457e50 rather than the
   base_action op. */
int32_t aoe_mp_setup_ui_idle_ror(AoeUiPanelRor *p);
int32_t aoe_mp_setup_ui_action_ror(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code,
                                   int32_t value, int32_t extra);

#endif
