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

#endif
