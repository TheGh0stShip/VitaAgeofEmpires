/* Patched RoR 1.0a scrollbar panel 0x00466e30..0x00467af0. */
#include "ui_scrollbar.h"

#include <assert.h>
#include <string.h>

static uint32_t clock_ms;
static uint32_t now(void *c) { (void)c; return clock_ms; }

static int32_t last_code = -1, last_pos = -1, calls;
static int32_t record(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code,
                      int32_t pos, int32_t extra) {
    (void)p; (void)from; (void)extra;
    last_code = code;
    last_pos = pos;
    ++calls;
    return 1;
}

int main(void) {
    AoeUiPlatformRor pf = {0};
    static AoeUiSystemRor sys;
    AoeUiAreaRor area = {(void *)1, 640, 480};
    static AoeUiPanelRor parent;
    static AoeUiPanelVtblRor parent_vt;
    static AoeUiScrollbarRor s, h;

    pf.now_ms = now;
    sys.platform = &pf;
    aoe_ui_panel_init_ror(&parent, &sys);
    parent_vt = aoe_ui_panel_vtable_ror;
    parent_vt.action = record;
    parent.vt = &parent_vt;
    aoe_ui_panel_setup_ror(&parent, &area, NULL, 0, 0, 640, 480, 0);

    aoe_ui_scrollbar_init_ror(&s, &sys);
    assert(s.repeat_interval == 100 && s.repeat_delay == 250 &&
           s.bevel_type == 1 && s.panel.field_ee == 5);

    /* A vertical bar 20 wide and 100 tall: arrows are width-sized squares
       inset by 2, the thumb travels between them. */
    assert(aoe_ui_scrollbar_setup_ror(&s, &area, &parent, 0, 0, 20, 100, 0, 0,
                                      NULL, 10, 0));
    s.panel.vt->set_rect(&s.panel, 0, 0, 20, 100);
    assert(s.back.left == 2 && s.back.top == 2 && s.back.right == 17 &&
           s.back.bottom == 17);
    assert(s.forward.top == 82 && s.forward.bottom == 97);
    assert(s.thumb_min_y == 18 && s.thumb_max_y == 66 && s.thumb.top == 18 &&
           s.thumb.bottom == 33 && s.page_forward.top == 34 &&
           s.page_forward.bottom == 81);

    /* Position maps onto the track; the last position pins to the end. */
    aoe_ui_scrollbar_set_range_ror(&s, 10, 5);
    assert(s.position == 5 && s.thumb_y == 42 && s.page_back.bottom == 41);
    aoe_ui_scrollbar_set_position_ror(&s, 9);
    assert(s.thumb_y == 66);

    /* Without a target the bar moves itself and tells its parent. */
    s.line_step = 1;
    s.page_step = 3;
    aoe_ui_scrollbar_set_position_ror(&s, 5);
    aoe_ui_scrollbar_scroll_ror(&s, 0, 0);
    assert(s.position == 4 && last_code == 4 && last_pos == 4);
    aoe_ui_scrollbar_scroll_ror(&s, 3, 0);
    assert(s.position == 7);
    aoe_ui_scrollbar_scroll_ror(&s, 3, 0);
    assert(s.position == 9);
    calls = 0;
    aoe_ui_scrollbar_scroll_ror(&s, 1, 0); /* clamped, unchanged: silent */
    assert(s.position == 9 && calls == 0);
    aoe_ui_scrollbar_scroll_ror(&s, 4, -5);
    assert(s.position == 0);
    aoe_ui_scrollbar_scroll_ror(&s, 9, 3); /* unknown code */
    assert(s.position == 0);

    /* Pressing the forward arrow steps once, then auto-repeats after the
       delay at the interval. */
    clock_ms = 1000;
    s.panel.vt->actions[AOE_UI_LEFT_DOWN](&s.panel, 5, 90, 0, 0);
    assert(s.part == 2 && s.repeating && s.repeat_state == 1 &&
           s.position == 1 && sys.capture == &s.panel);
    clock_ms = 1200;
    s.panel.vt->handle_idle(&s.panel);
    assert(s.repeat_state == 1 && s.position == 1);
    clock_ms = 1250;
    s.panel.vt->handle_idle(&s.panel);
    assert(s.repeat_state == 2 && s.repeat_ms == 1150);
    s.panel.vt->handle_idle(&s.panel);
    assert(s.position == 2 && s.repeat_ms == 1250);
    s.panel.vt->handle_idle(&s.panel);
    assert(s.position == 2);
    /* Dragging off the arrow stops the repeat; back on re-arms it. */
    s.panel.vt->actions[AOE_UI_LEFT_DRAG](&s.panel, 5, 50, 0, 0);
    assert(!s.repeating && s.repeat_state == 0);
    s.panel.vt->actions[AOE_UI_LEFT_DRAG](&s.panel, 5, 90, 0, 0);
    assert(s.repeating && s.repeat_state == 2 && s.position == 3);
    s.panel.vt->actions[AOE_UI_LEFT_UP](&s.panel, 5, 90, 0, 0);
    assert(last_code == 5 && last_pos == 3 && s.part == 0 && !s.repeating &&
           sys.capture == NULL);

    /* Grabbing the thumb and dragging maps the thumb back to a position. */
    aoe_ui_scrollbar_set_position_ror(&s, 0);
    s.panel.vt->actions[AOE_UI_LEFT_DOWN](&s.panel, 5, 20, 0, 0);
    assert(s.part == 3 && s.grab_y == 2 && !s.repeating);
    s.panel.vt->actions[AOE_UI_LEFT_DRAG](&s.panel, 5, 44, 0, 0);
    assert(s.thumb_y == 42 && s.position == 5 && last_pos == 5);
    s.panel.vt->actions[AOE_UI_LEFT_DRAG](&s.panel, 5, 200, 0, 0);
    assert(s.thumb_y == 66 && s.position == 9);
    s.panel.vt->actions[AOE_UI_LEFT_UP](&s.panel, 5, 200, 0, 0);

    /* The page areas repeat only while the pointer stays on them. */
    aoe_ui_scrollbar_set_position_ror(&s, 0);
    clock_ms = 5000;
    s.panel.vt->actions[AOE_UI_LEFT_DOWN](&s.panel, 5, 60, 0, 0);
    assert(s.part == 5 && s.position == 3);
    s.mouse_y = 20; /* now over the thumb */
    clock_ms = 5300;
    s.panel.vt->handle_idle(&s.panel);
    s.panel.vt->handle_idle(&s.panel);
    assert(!s.repeating && s.repeat_state == 0 && s.position == 3);
    s.panel.vt->actions[AOE_UI_LEFT_UP](&s.panel, 5, 20, 0, 0);

    /* With a target the request is forwarded untouched. */
    {
        static AoeUiPanelRor target;
        static AoeUiPanelVtblRor tvt;
        aoe_ui_panel_init_ror(&target, &sys);
        tvt = aoe_ui_panel_vtable_ror;
        tvt.action = record;
        target.vt = &tvt;
        s.target = &target;
        calls = 0;
        aoe_ui_scrollbar_scroll_ror(&s, 2, 77);
        assert(calls == 1 && last_code == 2 && last_pos == 77 && s.position == 3);
        s.target = NULL;
        aoe_ui_panel_release_ror(&target);
    }

    /* A horizontal bar uses the height for its squares. */
    aoe_ui_scrollbar_init_ror(&h, &sys);
    aoe_ui_scrollbar_setup_ror(&h, &area, &parent, 10, 0, 100, 20, 0, 0, NULL,
                               4, 1);
    h.panel.vt->set_rect(&h.panel, 10, 0, 100, 20);
    assert(h.back.left == 12 && h.back.right == 27 && h.forward.left == 92 &&
           h.forward.right == 107 && h.thumb_min_x == 28 &&
           h.thumb_max_x == 76 && h.thumb.right == 43);
    aoe_ui_scrollbar_set_position_ror(&h, 2);
    assert(h.thumb_x == 52 && h.page_back.right == 51);
    {
        const uint8_t colors[6] = {1, 2, 3, 4, 5, 6};
        int shape;
        aoe_ui_scrollbar_set_bevel_ror(&h, 3, colors);
        aoe_ui_scrollbar_set_pictures_ror(&h, &shape, 9, 8, 7, 5);
        assert(h.bevel_type == 3 && h.bevel[5] == 6 &&
               h.picture == &shape && h.frames[3] == 5);
    }

    aoe_ui_scrollbar_release_ror(&h);
    aoe_ui_scrollbar_release_ror(&s);
    aoe_ui_panel_release_ror(&parent);
    return 0;
}
