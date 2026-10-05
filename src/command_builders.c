#include "command_builders.h"

#include <string.h>

#include "aoe_ftol.h"
#include "le_bytes.h"

static int32_t object_id(const AoeCommandObjectRor *object) {
    return object ? object->id : -1;
}

static void put_ids(uint8_t *out, const AoeCommandObjectRor *const *units,
                    int32_t count) {
    for (int32_t i = 0; i < count; ++i)
        aoe_le_put_u32(out + 4 * i, (uint32_t)object_id(units[i]));
}

size_t aoe_command_build_order_ror(uint8_t *out, size_t out_size,
                                   const AoeCommandObjectRor *const *units,
                                   int16_t count,
                                   const AoeCommandObjectRor *target,
                                   float x, float y,
                                   const AoeCommandSessionRor *session) {
    size_t size = 0x1c + 4u * (size_t)(count > 0 ? count : 0);
    if (!units || count <= 0 || !out || out_size < size) return 0;
    memset(out, 0, size);
    put_ids(out + 0x1c, units, count);
    aoe_le_put_u32(out + 4, (uint32_t)object_id(target));
    out[0x11] = target ? target->template_65 : 0;
    out[0] = 0;
    out[1] = (uint8_t)count;
    aoe_le_put_f32(out + 8, x);
    aoe_le_put_f32(out + 0xc, y);
    if (session && session->session) {
        out[0x10] = session->local_player;
        out[0x13] = 0;
        out[0x12] = session->player_104;
        aoe_le_put_u32(out + 0x14, session->mask);
        aoe_le_put_u32(out + 0x18,
                       session->cell ? session->cell(session->context,
                                                     aoe_ftol32(y),
                                                     aoe_ftol32(x))
                                     : 0);
    }
    return size;
}

size_t aoe_command_build_order_alternate_ror(
    uint8_t *out, size_t out_size, const AoeCommandObjectRor *const *units,
    int16_t count, const AoeCommandObjectRor *target, float x, float y) {
    size_t size = 0x1c + 4u * (size_t)(count > 0 ? count : 0);
    if (!units || count <= 0 || !out || out_size < size) return 0;
    memset(out, 0, size);
    put_ids(out + 0x1c, units, count);
    aoe_le_put_u32(out + 4, (uint32_t)object_id(target));
    aoe_le_put_f32(out + 0xc, y);
    out[0] = 2;
    out[1] = (uint8_t)count;
    aoe_le_put_f32(out + 8, x);
    return size;
}

size_t aoe_command_build_order_alternate_ids_ror(uint8_t *out,
                                                 size_t out_size,
                                                 const int32_t *ids,
                                                 int32_t count,
                                                 int32_t target_id, float x,
                                                 float y) {
    size_t size = 0x1c + 4u * (size_t)(count > 0 ? count : 0);
    if (!ids || count <= 0 || !out || out_size < size) return 0;
    memset(out, 0, size);
    for (int32_t i = 0; i < count; ++i)
        aoe_le_put_u32(out + 0x1c + 4 * i, (uint32_t)ids[i]);
    aoe_le_put_u32(out + 4, (uint32_t)target_id);
    aoe_le_put_f32(out + 0xc, y);
    out[0] = 2;
    out[1] = (uint8_t)count;
    aoe_le_put_f32(out + 8, x);
    return size;
}

size_t aoe_command_build_arrange_ror(uint8_t *out, size_t out_size,
                                     const AoeCommandObjectRor *const *units,
                                     uint32_t count, int32_t pattern) {
    size_t size;
    if (!units || (int32_t)count <= 0 || !out) return 0;
    size = 0xc + 4u * (size_t)count;
    if (out_size < size) return 0;
    memset(out, 0, size);
    put_ids(out + 0xc, units, (int32_t)count);
    aoe_le_put_u32(out + 8, (uint32_t)pattern);
    out[0] = 6;
    aoe_le_put_u32(out + 4, count & 0xffu);
    return size;
}

size_t aoe_command_build_task_list_ror(
    uint8_t *out, size_t out_size, const AoeCommandObjectRor *const *units,
    int16_t count, const AoeCommandTaskFieldsRor *f) {
    size_t size = 0x24 + 4u * (size_t)(count > 0 ? count : 0);
    if (!units || count <= 0 || !f || !out || out_size < size) return 0;
    memset(out, 0, size);
    put_ids(out + 0x24, units, count);
    out[1] = f->player;
    out[2] = f->mode;
    out[4] = (uint8_t)f->value_16;
    out[5] = (uint8_t)((uint16_t)f->value_16 >> 8);
    out[6] = f->extra_c;
    aoe_le_put_u32(out + 8, (uint32_t)f->value_32);
    out[0xc] = f->flag;
    aoe_le_put_u32(out + 0x10, f->x_bits);
    aoe_le_put_u32(out + 0x14, f->y_bits);
    aoe_le_put_u32(out + 0x18, f->z_bits);
    aoe_le_put_u32(out + 0x1c, f->extra_32);
    out[0x20] = f->extra_a;
    out[0] = 0x11;
    out[3] = (uint8_t)count;
    out[0x21] = f->extra_b;
    return size;
}

size_t aoe_command_build_resign_ror(uint8_t *out, size_t out_size,
                                    uint8_t player, uint8_t slot,
                                    uint8_t value) {
    if (!out || out_size < 4) return 0;
    out[1] = player;
    out[0] = 0x0b;
    out[3] = value;
    out[2] = slot;
    return 4;
}

size_t aoe_command_build_unit_records_ror(uint8_t *out, size_t out_size,
                                          uint8_t player, int32_t unit_id,
                                          const uint8_t *records,
                                          int32_t count) {
    if (!out || out_size < 0x18 || count > 5 || (count > 0 && !records))
        return 0;
    memset(out, 0, 0x18);
    out[1] = player;
    out[0] = 0x0c;
    aoe_le_put_u32(out + 4, (uint32_t)unit_id);
    out[8] = (uint8_t)count;
    for (int32_t i = 0; i < count; ++i) memcpy(out + 9 + 3 * i, records + 3 * i, 3);
    return 0x18;
}

size_t aoe_command_build_record_list_ror(
    uint8_t *out, size_t out_size, const AoeCommandObjectRor *const *units,
    int16_t count, uint8_t a, uint8_t b) {
    size_t size = 4 + 4u * (size_t)(count > 0 ? count : 0);
    if (!units || count <= 0 || !out || out_size < size) return 0;
    memset(out, 0, size);
    put_ids(out + 4, units, count);
    out[2] = a;
    out[0] = 0x10;
    out[1] = (uint8_t)count;
    out[3] = b;
    return size;
}

int aoe_command_same_order_ror(const uint8_t *a, const uint8_t *b) {
    uint8_t count;
    if (!a || !b || a[1] != b[1]) return 0;
    count = a[1];
    if ((int16_t)aoe_ftol32(aoe_le_f32(a + 8)) !=
            (int16_t)aoe_ftol32(aoe_le_f32(b + 8)) ||
        (int16_t)aoe_ftol32(aoe_le_f32(a + 0xc)) !=
            (int16_t)aoe_ftol32(aoe_le_f32(b + 0xc)) ||
        aoe_le_u32(a + 4) != aoe_le_u32(b + 4)) return 0;
    for (unsigned i = 0; i < count; ++i)
        if (aoe_le_u32(a + 0x1c + 4 * i) != aoe_le_u32(b + 0x1c + 4 * i))
            return 0;
    return 1;
}

int aoe_command_same_stop_ror(const uint8_t *a, const uint8_t *b) {
    if (!a || !b || a[1] != b[1]) return 0;
    for (unsigned i = 0; i < a[1]; ++i)
        if (aoe_le_u32(a + 2 + 4 * i) != aoe_le_u32(b + 2 + 4 * i)) return 0;
    return 1;
}

int aoe_command_same_ror(const uint8_t *last, const uint8_t *command) {
    if (!last || !command) return 0;
    switch (command[0]) {
    case 0: case 2: case 3: return aoe_command_same_order_ror(command, last);
    case 1: return aoe_command_same_stop_ror(command, last);
    default: return 0;
    }
}

int aoe_command_is_repeat_ror(const AoeCommandRepeatRor *r,
                              const uint8_t *command, size_t size,
                              uint32_t now_ms) {
    size_t need;
    if (!r || !r->last || !command || r->last_size != size || !size ||
        r->last[0] != command[0]) return 0;
    if ((int32_t)(now_ms - r->time_ms) >= 500) return 0;
    switch (command[0]) {
    case 0: case 2: case 3: need = 0x1c; break;
    case 1: need = 2; break;
    default: return 0;
    }
    /* Host bound: both records must hold the IDs they count. */
    if (size < need + 4u * command[1] || size < need + 4u * r->last[1])
        return 0;
    return aoe_command_same_ror(r->last, command);
}
