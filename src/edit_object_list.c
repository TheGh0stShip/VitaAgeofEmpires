/* Patched RoR 1.0a scenario editor object list dialog, vtable 0x00545b84
   (0x0046a890..0x0046ad40). */
#include "edit_object_list.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "ui_ez_slots.h"

#define OBJ(p) ((AoeEditObjectListRor *)(p))

static void object_list_destroy(AoeUiPanelRor *p) {
    aoe_edit_object_list_release_ror(OBJ(p));
}

const AoeUiEzVtblRor aoe_edit_object_list_vtable_ror = {
    AOE_UI_EZ_SLOTS(object_list_destroy, aoe_ui_panel_handle_idle_ror,
                    aoe_ui_panel_handle_paint_ror, aoe_ui_ez_no_key_action_ror,
                    aoe_ui_panel_zero_ror, aoe_ui_list_dialog_action_slot_ror,
                    aoe_ui_ez_set_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};

int aoe_edit_object_list_init_ror(AoeEditObjectListRor *d,
                                  AoeUiSystemRor *system,
                                  const AoeUiEzOpsRor *ops,
                                  const AoeEditObjectListOpsRor *list_ops,
                                  const char *name) {
    int ok;
    if (!d) return 0;
    ok = aoe_ui_list_dialog_init_ror(&d->list, system, ops, name);
    d->list.dialog.ez.panel.vt = &aoe_edit_object_list_vtable_ror.base;
    d->list_ops = list_ops;
    return ok;
}

/* The original strcpy()s the template name into the 100-byte entry; the
   port truncates instead of overrunning (host-safety difference only for
   names of 100 bytes or more). */
static void copy_name(char *out, const char *name) {
    size_t n = name ? strlen(name) : 0;
    if (n > 99) n = 99;
    if (n) memcpy(out, name, n);
    out[n] = '\0';
}

int aoe_edit_object_list_build_ror(AoeEditObjectListRor *d) {
    const AoeEditObjectListOpsRor *o = d->list_ops;
    const AoePlayerTemplates *t;
    int16_t at = 1;
    uint16_t i;
    if (d->mode != 1) return 0;
    t = o && o->templates ? o->templates(o->context) : NULL;
    if (!t) return 0;
    d->count = 1;
    for (i = 0; i < t->count; ++i)
        if (t->slots[i] && !t->slots[i]->hide_in_editor) ++d->count;
    if (d->count == 0) return 0;
    d->entries = calloc((size_t)d->count, sizeof(*d->entries));
    if (!d->entries) return 0;
    d->entries[0].id = -1;
    strcpy(d->entries[0].name, " <None> ");
    for (i = 0; i < t->count; ++i) {
        const DatUnit *u = t->slots[i];
        AoeEditObjectEntryRor *e;
        if (!u || u->hide_in_editor) continue;
        e = &d->entries[at++];
        e->id = (int16_t)i;
        if ((int16_t)u->language_name_id < 1)
            copy_name(e->name, u->name);
        else if (o->object_name)
            o->object_name(o->context, (int16_t)u->language_name_id, e->name,
                           100);
    }
    return 1;
}

/* 0x00492c30: CompareStringA(LOCALE_USER_DEFAULT, NORM_IGNORECASE);
   less is -1, greater 1, equal 0. */
static int compare_entries(const void *a, const void *b) {
    const unsigned char *x = (const unsigned char *)((const AoeEditObjectEntryRor *)a)->name;
    const unsigned char *y = (const unsigned char *)((const AoeEditObjectEntryRor *)b)->name;
    for (;; ++x, ++y) {
        int cx = tolower(*x), cy = tolower(*y);
        if (cx != cy) return cx < cy ? -1 : 1;
        if (!cx) return 0;
    }
}

void aoe_edit_object_list_sort_ror(AoeEditObjectListRor *d) {
    if (d->entries && d->count > 0)
        qsort(d->entries, (size_t)d->count, sizeof(*d->entries),
              compare_entries);
}

void aoe_edit_object_list_find_default_ror(AoeEditObjectListRor *d) {
    int16_t id, i;
    d->default_index = -1;
    id = aoe_edit_object_list_default_id_ror(d);
    if (id != -1) {
        for (i = 0; i < d->count; ++i)
            if (d->entries[i].id == id) {
                d->default_index = i;
                break;
            }
    }
    if (d->default_index == -1) d->default_index = 0;
}

int16_t aoe_edit_object_list_default_id_ror(const AoeEditObjectListRor *d) {
    const AoeEditObjectListOpsRor *o = d->list_ops;
    if (d->mode != 1) return -1;
    return o && o->place_object ? o->place_object(o->context) : -1;
}

int16_t aoe_edit_object_list_choice_ror(const AoeEditObjectListRor *d) {
    if (d->list.selected == -1) return -1;
    return d->entries[d->list.selected].id;
}

int aoe_edit_object_list_setup_ror(AoeEditObjectListRor *d,
                                   AoeUiPanelRor *parent,
                                   const char *info_name, int32_t info_id,
                                   int32_t mode, const char *title) {
    int16_t i;
    d->mode = mode;
    d->entries = NULL;
    d->lines = NULL;
    if (d->list.dialog.ez.panel.field_d8) return 0;
    if (!aoe_edit_object_list_build_ror(d)) return 0;
    aoe_edit_object_list_sort_ror(d);
    aoe_edit_object_list_find_default_ror(d);
    d->lines = calloc((size_t)d->count, sizeof(*d->lines));
    if (!d->lines) return 0;
    for (i = 0; i < d->count; ++i) {
        size_t n = strlen(d->entries[i].name) + 1;
        d->lines[i] = calloc(n, 1);
        if (!d->lines[i]) return 0;
        memcpy(d->lines[i], d->entries[i].name, n);
    }
    return aoe_ui_list_dialog_setup_ror(
        &d->list, parent, info_name, info_id, 0x136, 400, 10, 10, 0x122, 0x19,
        title, 10, 0x2d, 0x122, 0x131, (const char *const *)d->lines,
        d->count, d->default_index, 10, 0xa0, 0x168, 0x8c, 0x1e);
}

void aoe_edit_object_list_release_ror(AoeEditObjectListRor *d) {
    int16_t i;
    if (d->lines) {
        for (i = 0; i < d->count; ++i) free(d->lines[i]);
        free(d->lines);
        d->lines = NULL;
    }
    if (d->entries) {
        free(d->entries);
        d->entries = NULL;
    }
    aoe_ui_list_dialog_destroy_slot_ror(&d->list.dialog.ez.panel);
}
