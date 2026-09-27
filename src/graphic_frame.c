#include "graphic_frame.h"

#include <limits.h>

int aoe_graphic_select_frame(const DatGraphic *graphic,
                             uint16_t angle, int16_t animation_frame,
                             AoeGraphicFrame *out) {
    if (!graphic || !graphic->present || !out ||
        !graphic->frame_count || !graphic->angle_count ||
        angle >= graphic->angle_count)
        return 0;
    int32_t frame = animation_frame;
    if (frame < 0 || frame >= graphic->frame_count)
        frame = 0;
    AoeGraphicFrame selected = {0};
    uint16_t count = graphic->angle_count;
    uint8_t mirror = graphic->mirror_mode;
    int64_t index;
    if (mirror && count == 2) {
        selected.slp_frame = (uint32_t)frame;
        selected.mirror_horizontal = (uint8_t)(angle & 0xff);
        *out = selected;
        return 1;
    }
    if (mirror && (mirror < angle || angle < count / 4)) {
        int32_t half = count / 2;
        int32_t mirrored_angle = half < angle ? half - angle + count
                                               : half - angle;
        index = (int64_t)(mirrored_angle - count / 4) *
                graphic->frame_count + frame;
        selected.mirror_horizontal = 1;
    } else if (!mirror) {
        index = (int64_t)graphic->frame_count * angle + frame;
    } else {
        index = (int64_t)((int32_t)angle - count / 4) *
                graphic->frame_count + frame;
    }
    if (index < 0 || index > UINT32_MAX)
        return 0;
    selected.slp_frame = (uint32_t)index;
    *out = selected;
    return 1;
}
