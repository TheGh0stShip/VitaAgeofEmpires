#include "ui_picture.h"

#include <stdio.h>
#include <string.h>

#define P(p) ((AoeUiPictureRor *)(p))

void aoe_ui_picture_init_ror(AoeUiPictureRor *pic, AoeUiSystemRor *system,
                             const AoeUiPictureOpsRor *ops) {
    if (!pic) return;
    memset(pic, 0, sizeof(*pic));
    aoe_ui_panel_init_ror(&pic->panel, system);
    pic->panel.vt = &aoe_ui_picture_vtable_ror;
    pic->ops = ops;
    pic->keep = 1;
    pic->panel.handles_mouse = 0;
    pic->panel.field_ee = 2;
    pic->id = -1;
}

void aoe_ui_picture_free_ror(AoeUiPictureRor *pic) {
    const AoeUiPictureOpsRor *ops;
    if (!pic) return;
    ops = pic->ops;
    if (pic->bitmap) {
        if (pic->loaded && ops && ops->free_bitmap) ops->free_bitmap(ops->context, pic->bitmap);
        pic->bitmap = NULL;
    }
    if (pic->shape) {
        if (pic->loaded && ops && ops->free_shape) ops->free_shape(ops->context, pic->shape);
        pic->shape = NULL;
    }
    pic->loaded = 0;
}

void aoe_ui_picture_release_ror(AoeUiPictureRor *pic) {
    if (!pic) return;
    aoe_ui_picture_free_ror(pic);
    aoe_ui_panel_release_ror(&pic->panel);
}

static void destroy(AoeUiPanelRor *p) { aoe_ui_picture_release_ror(P(p)); }

int aoe_ui_picture_load_ror(AoeUiPictureRor *pic) {
    const AoeUiPictureOpsRor *ops;
    char file[80];
    if (!pic) return 0;
    ops = pic->ops;
    aoe_ui_picture_free_ror(pic);
    if (!ops) return 0;
    if (!pic->name[0]) {
        if (pic->id == -1) return 0;
        pic->shape = ops->load_shape ? ops->load_shape(ops->context, "", pic->id) : NULL;
    } else {
        snprintf(file, sizeof file, "%s.shp", pic->name);
        pic->shape = ops->load_shape ? ops->load_shape(ops->context, file, pic->id) : NULL;
    }
    if (pic->shape) {
        if (ops->shape_valid && ops->shape_valid(ops->context, pic->shape)) {
            pic->is_shape = 1;
            pic->loaded = 1;
            return 1;
        }
        if (ops->free_shape) ops->free_shape(ops->context, pic->shape);
        pic->shape = NULL;
    }
    snprintf(file, sizeof file, "%s.bmp", pic->name);
    pic->bitmap = ops->load_bitmap ? ops->load_bitmap(ops->context, file) : NULL;
    if (pic->bitmap) {
        pic->is_shape = 0;
        pic->loaded = 1;
        return 1;
    }
    return 0;
}

int aoe_ui_picture_setup_ror(AoeUiPictureRor *pic, AoeUiAreaRor *area,
                             AoeUiPanelRor *parent, int32_t x, int32_t y,
                             int32_t w, int32_t h, const char *name,
                             int32_t id, int32_t fit, int32_t keep) {
    if (!pic) return 0;
    if (!name) {
        pic->name[0] = 0;
    } else {
        strncpy(pic->name, name, sizeof pic->name - 1);
        pic->name[sizeof pic->name - 1] = 0;
    }
    pic->id = id;
    pic->fit = fit;
    pic->keep = keep;
    if ((fit || keep) && aoe_ui_picture_load_ror(pic)) {
        if (pic->fit) {
            if (!pic->is_shape) {
                if (pic->ops->bitmap_size) pic->ops->bitmap_size(pic->ops->context, pic->bitmap, &w, &h);
            } else {
                int16_t sw = 0, sh = 0;
                if (pic->ops->shape_size) pic->ops->shape_size(pic->ops->context, pic->shape, 0, &sw, &sh);
                w = sw;
                h = sh;
            }
        }
        if (!pic->keep) aoe_ui_picture_free_ror(pic);
    }
    aoe_ui_panel_setup_ror(&pic->panel, area, parent, x, y, w, h, 0);
    return 1;
}

static void draw(AoeUiPanelRor *p) {
    AoeUiPictureRor *pic = P(p);
    const AoeUiPictureOpsRor *ops = pic->ops;
    if (!p->area || !p->visible || !p->active) return;
    p->vt->begin_draw(p, 0);
    if (!pic->keep) aoe_ui_picture_load_ror(pic);
    if (ops) {
        if (!pic->is_shape) {
            if (pic->bitmap && ops->draw_bitmap)
                ops->draw_bitmap(ops->context, p->area, pic->bitmap, p->x, p->y);
        } else if (pic->shape && ops->draw_shape) {
            ops->draw_shape(ops->context, p->area, pic->shape, p->x, p->y, 0);
        }
    }
    if (!pic->keep) aoe_ui_picture_free_ror(pic);
    p->vt->end_draw(p);
}

static int32_t no_key_action(AoeUiPanelRor *p, int32_t a, int32_t b,
                             int32_t c, int32_t d, int32_t e) {
    (void)p; (void)a; (void)b; (void)c; (void)d; (void)e;
    return 0;
}

const AoeUiPanelVtblRor aoe_ui_picture_vtable_ror = {
    destroy,
    aoe_ui_panel_setup_ror,
    aoe_ui_panel_set_rect_ltrb_ror,
    aoe_ui_panel_set_rect_ror,
    aoe_ui_panel_set_color_ror,
    aoe_ui_panel_set_active_ror,
    aoe_ui_panel_set_position_mode_ror,
    aoe_ui_panel_set_fixed_position_ror,
    aoe_ui_panel_set_redraw_ror,
    aoe_ui_panel_redraw_overlaps_ror,
    aoe_ui_panel_begin_draw_ror,
    aoe_ui_panel_end_draw_ror,
    draw,
    aoe_ui_panel_draw_rect_ror,
    aoe_ui_panel_draw_offset_ror,
    aoe_ui_panel_draw_parent_rect_ror,
    aoe_ui_panel_draw_parent_offset_ror,
    aoe_ui_panel_validate_ror,
    aoe_ui_panel_handle_message_ror,
    aoe_ui_panel_handle_idle_ror,
    aoe_ui_panel_handle_size_ror,
    aoe_ui_panel_handle_paint_ror,
    aoe_ui_panel_key_down_ror,
    aoe_ui_panel_char_ror,
    aoe_ui_panel_ignore_ror,
    aoe_ui_panel_ignore_ror,
    aoe_ui_panel_ignore_ror,
    aoe_ui_panel_ignore_ror,
    aoe_ui_panel_mouse_down_ror,
    aoe_ui_panel_mouse_move_ror,
    aoe_ui_panel_mouse_up_ror,
    aoe_ui_panel_mouse_dbl_ror,
    {aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror},
    no_key_action,
    aoe_ui_panel_zero_ror,
    aoe_ui_panel_no_char_action_ror,
    aoe_ui_panel_action_ror,
    aoe_ui_panel_get_true_rect_ror,
    aoe_ui_panel_is_inside_ror,
    aoe_ui_panel_set_focus_ror,
    aoe_ui_panel_set_tab_order_ror,
    aoe_ui_panel_get_help_ror,
    aoe_ui_panel_stop_sound_ror,
    aoe_ui_panel_restart_sound_ror,
    aoe_ui_panel_slot_e0_ror,
};
