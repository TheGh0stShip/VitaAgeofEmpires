/* Patched RoR 1.0a game screen: the edit mode change notification
   0x00481690 (the game's 0x004ff420 calls it with the old and new edit
   mode). */
#include "game_screen.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static char log_[256];
static void *sync_toolbar;

static void say(const char *t) {
    size_t n = strlen(log_);
    snprintf(log_ + n, sizeof log_ - n, "%s", t);
}

static void refresh(void *c) {
    (void)c;
    say("B ");
}

static void toolbar_sync(void *c, void *toolbar, int32_t old_mode, int32_t mode) {
    char t[64];
    (void)c;
    sync_toolbar = toolbar;
    snprintf(t, sizeof t, "T %d %d ", (int)old_mode, (int)mode);
    say(t);
}

static AoeGameScreenOpsRor ops;

static void setup(AoeGameScreenRor *s, int toolbar, int button) {
    memset(s, 0, sizeof *s);
    s->ops = &ops;
    s->panel_57c = toolbar ? (void *)0x57c : NULL;
    s->buttons[0] = button ? (void *)0x4c8 : NULL;
    s->buttons[1] = (void *)0x4cc;
    s->button_page = 3;
    s->shown_page = 2;
    s->button_pages = 4;
    log_[0] = '\0';
    sync_toolbar = NULL;
}

int main(void) {
    AoeGameScreenRor s;
    memset(&ops, 0, sizeof ops);
    ops.refresh_buttons = refresh;
    ops.toolbar_sync = toolbar_sync;

    /* Toolbar and buttons: the toolbar follows (old, new) first, then the
       first page is requested and the buttons rebuilt. */
    setup(&s, 1, 1);
    aoe_game_screen_mode_changed_ror(&s, 0, 0x12);
    assert(strcmp(log_, "T 0 18 B ") == 0);
    assert(sync_toolbar == (void *)0x57c);
    assert(s.button_page == 0 && s.shown_page == 2 && s.button_pages == 4);

    /* The arguments pass through in order (leaving mode 1 for 0). */
    setup(&s, 1, 1);
    aoe_game_screen_mode_changed_ror(&s, 1, 0);
    assert(strcmp(log_, "T 1 0 B ") == 0);

    /* No toolbar (+0x57c clear): only the button page. */
    setup(&s, 0, 1);
    aoe_game_screen_mode_changed_ror(&s, 6, 1);
    assert(strcmp(log_, "B ") == 0 && sync_toolbar == NULL && s.button_page == 0);

    /* No first button (+0x4c8 clear; a later button does not count): the
       toolbar only, the requested page kept. */
    setup(&s, 1, 0);
    aoe_game_screen_mode_changed_ror(&s, 2, 3);
    assert(strcmp(log_, "T 2 3 ") == 0 && s.button_page == 3);

    /* Neither: nothing. */
    setup(&s, 0, 0);
    aoe_game_screen_mode_changed_ror(&s, 0x15, 0);
    assert(log_[0] == '\0' && s.button_page == 3);

    puts("game screen notify ok");
    return 0;
}
