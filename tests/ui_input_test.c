/* Patched RoR 1.0a text input panel 0x00463650..0x00464910. */
#include "ui_input.h"

#include <assert.h>
#include <string.h>

static uint32_t clock_ms;
static uint32_t now(void *c) { (void)c; return clock_ms; }
static int32_t width(void *c, uintptr_t f, const char *t, int32_t n) { (void)c; (void)f; (void)t; return n * 8; }
static int beeps;
static void beep(void *c, void *s) { (void)c; (void)s; ++beeps; }
static const char *clip_text = "XY";
static int32_t clip(void *c, char *out, int32_t size) {
    (void)c;
    strncpy(out, clip_text, (size_t)size - 1);
    return (int32_t)strlen(clip_text);
}
static int32_t codes[8], n_actions;
static int32_t record(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t v, int32_t e) {
    (void)p; (void)from; (void)v; (void)e; codes[n_actions++] = code; return 1;
}

static void type(AoeUiInputRor *in, const char *s) {
    for (; *s; ++s) in->text.panel.vt->char_action(&in->text.panel, (unsigned char)*s, 1);
}
static void key(AoeUiInputRor *in, int32_t k, int32_t ctrl, int32_t shift) {
    in->text.panel.vt->key_action(&in->text.panel, k, 1, 0, ctrl, shift);
}

int main(void) {
    AoeUiPlatformRor pf = {0};
    static AoeUiSystemRor sys;
    AoeUiAreaRor area = {(void *)1, 640, 480};
    AoeUiInputOpsRor ops = {NULL, beep, clip};
    static AoeUiPanelRor parent;
    static AoeUiPanelVtblRor pvt;
    static AoeUiInputRor in, num, multi;

    pf.now_ms = now;
    pf.text_width = width;
    sys.platform = &pf;
    aoe_ui_panel_init_ror(&parent, &sys);
    pvt = aoe_ui_panel_vtable_ror;
    pvt.action = record;
    parent.vt = &pvt;

    /* Free text, 8 characters. */
    aoe_ui_input_init_ror(&in, &sys, &ops, 500);
    assert(in.insert && in.caret_on && in.text.align == 5 && in.text.panel.field_ee == 7);
    assert(aoe_ui_input_setup_ror(&in, &area, &parent, 0, 0, 200, 20, 1, 8, 16,
                                  NULL, 0, 0, 0, 0, 0, 0, 4, 8, NULL, "ab", 0));
    assert(!strcmp(aoe_ui_input_text_ror(&in), "ab") && in.cursor == 2 &&
           in.text.column_chars == 2 && in.text.max_chars == 8);
    type(&in, "cd");
    assert(!strcmp(aoe_ui_input_text_ror(&in), "abcd") && in.cursor == 4);
    key(&in, 0x25, 0, 0);  /* Left */
    key(&in, 0x25, 0, 0);
    assert(in.cursor == 2);
    type(&in, "X");
    assert(!strcmp(aoe_ui_input_text_ror(&in), "abXcd") && in.cursor == 3);
    key(&in, 8, 0, 0);     /* Backspace */
    assert(!strcmp(aoe_ui_input_text_ror(&in), "abcd") && in.cursor == 2);
    key(&in, 0x2e, 0, 0);  /* Delete */
    assert(!strcmp(aoe_ui_input_text_ror(&in), "abd") && in.cursor == 2);
    key(&in, 0x24, 0, 0);  /* Home */
    assert(in.cursor == 0);
    key(&in, 0x23, 0, 0);  /* End */
    assert(in.cursor == 3);
    /* Shift+Insert and Ctrl+V paste; Insert alone toggles the flag. */
    key(&in, 0x2d, 0, 1);
    assert(!strcmp(aoe_ui_input_text_ror(&in), "abdXY") && in.cursor == 5);
    key(&in, 0x2d, 0, 0);
    assert(!in.insert);
    /* The buffer fills at 8: typing beeps, pasting past it beeps. */
    type(&in, "1234");
    assert(in.length == 8 && beeps == 1 && !strcmp(aoe_ui_input_text_ror(&in), "abdXY123"));
    clip_text = "";
    key(&in, 0x56, 1, 0);
    assert(beeps == 2);
    /* Control characters are ignored; Enter and Esc go to the parent. */
    in.text.panel.vt->char_action(&in.text.panel, 3, 1);
    assert(in.length == 8 && beeps == 2);
    key(&in, 0x0d, 0, 0);
    key(&in, 0x1b, 0, 0);
    assert(n_actions == 2 && codes[0] == 0 && codes[1] == 1);
    assert(!in.text.panel.vt->key_action(&in.text.panel, 'Q', 1, 0, 0, 0));

    /* Caret blink on focus. */
    in.text.panel.has_focus = 1;
    clock_ms = 1000;
    in.text.panel.vt->set_focus(&in.text.panel, 1);
    assert(in.caret_on && in.blink_at == 1000);
    clock_ms = 1499;
    in.text.panel.vt->handle_idle(&in.text.panel);
    assert(in.caret_on);
    clock_ms = 1500;
    in.text.panel.vt->handle_idle(&in.text.panel);
    assert(!in.caret_on && in.blink_at == 1500);

    /* Number formats. */
    aoe_ui_input_init_ror(&num, &sys, &ops, 500);
    assert(aoe_ui_input_setup_ror(&num, &area, &parent, 0, 30, 200, 20, 1, 8, 16,
                                  NULL, 0, 0, 0, 0, 0, 0, 4, 10, NULL, "", 1));
    beeps = 0;
    type(&num, "-12a.");
    /* '-' first is fine; 'a' is refused; the first '.' is refused too (the
       original only takes '.' once the text already holds one). */
    assert(!strcmp(aoe_ui_input_text_ror(&num), "-12") && beeps == 2);
    num.format = 3;
    aoe_ui_input_set_text_ror(&num.text, "");
    type(&num, "-5");
    assert(!strcmp(aoe_ui_input_text_ror(&num), "5"));
    num.format = 4; /* file name: one '.', no path characters */
    aoe_ui_input_set_text_ror(&num.text, "");
    type(&num, "a.b.c/:d");
    assert(!strcmp(aoe_ui_input_text_ror(&num), "a.bcd"));
    num.format = 6; /* path: ':' only as the second character */
    aoe_ui_input_set_text_ror(&num.text, "");
    type(&num, "c:\\x:");
    assert(!strcmp(aoe_ui_input_text_ror(&num), "c:\\x"));
    num.format = 8; /* percentage: two digits, or 100 */
    aoe_ui_input_set_text_ror(&num.text, "");
    type(&num, "100");
    assert(!strcmp(aoe_ui_input_text_ror(&num), "100"));
    aoe_ui_input_set_text_ror(&num.text, "");
    type(&num, "990");
    assert(!strcmp(aoe_ui_input_text_ror(&num), "99"));

    /* Multi-line: Enter inserts a newline; Up/Down keep the column. */
    aoe_ui_input_init_ror(&multi, &sys, &ops, 500);
    assert(aoe_ui_input_setup_ror(&multi, &area, &parent, 0, 60, 300, 100, 1, 8, 16,
                                  NULL, 0, 0, 0, 0, 0, 0, 4, 40, NULL, "", 7));
    assert(multi.text.wrap && multi.text.max_chars == 0 && multi.cursor == 0);
    type(&multi, "abc");
    key(&multi, 0x0d, 0, 0);
    type(&multi, "de");
    assert(!strcmp(aoe_ui_input_text_ror(&multi), "abc\nde"));
    assert(multi.text.count == 2 && multi.text.selected == 1 &&
           multi.text.column_chars == 2 && multi.cursor == 6);
    key(&multi, 0x26, 0, 0); /* Up */
    assert(multi.text.selected == 0 && multi.cursor == 2);
    key(&multi, 0x23, 1, 0); /* Ctrl+End */
    assert(multi.text.selected == 1 && multi.cursor == 6);
    key(&multi, 0x24, 1, 0); /* Ctrl+Home */
    assert(multi.cursor == 0 && multi.text.selected == 0);

    aoe_ui_input_release_ror(&multi);
    aoe_ui_input_release_ror(&num);
    aoe_ui_input_release_ror(&in);
    aoe_ui_panel_release_ror(&parent);
    return 0;
}
