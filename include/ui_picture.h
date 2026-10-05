#ifndef VITA_AOE_UI_PICTURE_H
#define VITA_AOE_UI_PICTURE_H

#include <stdint.h>

#include "ui_panel.h"

/* Patched Rise of Rome 1.0a picture panel (pnl_pic, vtable 0x005455d4,
   0x00466950..0x00466d60): a shape ("<name>.shp" or resource `id`) or,
   failing that, a bitmap ("<name>.bmp"), loaded once or around every
   draw. Loading and blitting go through AoeUiPictureOpsRor. */
typedef struct {
    void *context;
    /* 0x0049f5f0 shape load (name may be "") and 0x0049f9e0 validity;
       0x0049f840 free. */
    void *(*load_shape)(void *context, const char *name, int32_t id);
    int (*shape_valid)(void *context, void *shape);
    void (*free_shape)(void *context, void *shape);
    /* 0x0049fa00: frame size. */
    int (*shape_size)(void *context, void *shape, int32_t frame,
                      int16_t *w, int16_t *h);
    /* 0x0045af10 bitmap load (NULL or no pixels fail), size (+0x14,
       +0x18), 0x0045b0d0 free. */
    void *(*load_bitmap)(void *context, const char *name);
    void (*bitmap_size)(void *context, void *bitmap, int32_t *w, int32_t *h);
    void (*free_bitmap)(void *context, void *bitmap);
    /* 0x0045b370 / 0x0049fe00 at (x, y) in the area. */
    void (*draw_bitmap)(void *context, AoeUiAreaRor *area, void *bitmap,
                        int32_t x, int32_t y);
    void (*draw_shape)(void *context, AoeUiAreaRor *area, void *shape,
                       int32_t x, int32_t y, int32_t frame);
} AoeUiPictureOpsRor;

typedef struct {
    AoeUiPanelRor panel;
    char name[12];          /* +0xf4 (longer names are cut) */
    int32_t id;             /* +0x100, -1 */
    int32_t fit;            /* +0x104: take the picture's size */
    int32_t keep;           /* +0x108, 1: stay loaded */
    int32_t loaded;         /* +0x10c */
    int32_t is_shape;       /* +0x110 */
    void *bitmap;           /* +0x114 */
    void *shape;            /* +0x118 */
    const AoeUiPictureOpsRor *ops;
} AoeUiPictureRor;

extern const AoeUiPanelVtblRor aoe_ui_picture_vtable_ror;

/* 0x00466950 constructor; 0x004669d0 / 0x004669b0 destructors. */
void aoe_ui_picture_init_ror(AoeUiPictureRor *pic, AoeUiSystemRor *system,
                             const AoeUiPictureOpsRor *ops);
void aoe_ui_picture_release_ror(AoeUiPictureRor *pic);
/* 0x00466a20: name, resource ID, fit and keep, then the panel setup
   (fit takes the size from the picture; without keep it is freed again). */
int aoe_ui_picture_setup_ror(AoeUiPictureRor *pic, AoeUiAreaRor *area,
                             AoeUiPanelRor *parent, int32_t x, int32_t y,
                             int32_t w, int32_t h, const char *name,
                             int32_t id, int32_t fit, int32_t keep);
/* 0x00466b20 load (shape first, then bitmap), 0x00466cf0 free. */
int aoe_ui_picture_load_ror(AoeUiPictureRor *pic);
void aoe_ui_picture_free_ror(AoeUiPictureRor *pic);
/* +0x30 0x00466d60 draw. */

#endif
