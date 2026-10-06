/* The tech table as the executable reads it (patched RoR 1.0a 0x004e9c60):
   a 16-bit count, then per tech the four prerequisites, three costs (type,
   amount, paid byte), the required count, the research location (+0x28),
   the name and description string IDs (+0x2a, +0x2c), the research time
   (+0x1e), effect (+0x20), type (+0x22), icon (+0x24), button byte
   (+0x26), the help, tree-text and hotkey string IDs (+0x30, +0x34,
   +0x38) and a 16-bit-length name. */
#include "dat.h"

#include <stdlib.h>
#include <string.h>

static uint16_t le16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

void dat_free_techs(DatTechs *techs) {
    if (!techs) return;
    if (techs->techs)
        for (uint16_t i = 0; i < techs->count; ++i)
            free(techs->techs[i].name);
    free(techs->techs);
    memset(techs, 0, sizeof(*techs));
}

int dat_parse_techs(const DatBlob *blob, const DatCivilizations *civilizations,
                    DatTechs *techs) {
    size_t offset;
    uint16_t count;
    if (!techs) return 0;
    memset(techs, 0, sizeof(*techs));
    if (!blob || !blob->bytes || !civilizations ||
        civilizations->next_offset > blob->size ||
        blob->size - civilizations->next_offset < 2) return 0;
    offset = civilizations->next_offset;
    count = le16(blob->bytes + offset);
    offset += 2;
    if (count > 2048 || (size_t)count > (blob->size - offset) / 54)
        return 0;
    techs->techs = calloc(count ? count : 1, sizeof(*techs->techs));
    if (!techs->techs) return 0;
    techs->count = count;
    for (uint16_t i = 0; i < count; ++i) {
        DatTech *tech = &techs->techs[i];
        const uint8_t *p;
        uint16_t name_len;
        if (blob->size - offset < 54) goto fail;
        p = blob->bytes + offset;
        for (unsigned j = 0; j < 4; ++j)
            tech->required_techs[j] = (int16_t)le16(p + j * 2);
        for (unsigned j = 0; j < 3; ++j) {
            const uint8_t *cost = p + 8 + j * 5;
            tech->costs[j].resource_type = (int16_t)le16(cost);
            tech->costs[j].amount = (int16_t)le16(cost + 2);
            tech->costs[j].paid = (int8_t)cost[4];
        }
        tech->required_count = (int16_t)le16(p + 23);
        tech->research_location = (int16_t)le16(p + 25);
        tech->language_name = le16(p + 27);
        tech->language_description = le16(p + 29);
        tech->research_time = (int16_t)le16(p + 31);
        tech->effect_id = (int16_t)le16(p + 33);
        tech->field_22 = (int16_t)le16(p + 35);
        tech->field_24 = (int16_t)le16(p + 37);
        tech->button_id = (int8_t)p[39];
        tech->language_help = (int32_t)le32(p + 40);
        tech->language_tree = (int32_t)le32(p + 44);
        tech->hotkey = (int32_t)le32(p + 48);
        name_len = le16(p + 52);
        offset += 54;
        if (name_len > 4096 || name_len > blob->size - offset) goto fail;
        tech->name = malloc((size_t)name_len + 1);
        if (!tech->name) goto fail;
        memcpy(tech->name, blob->bytes + offset, name_len);
        tech->name[name_len] = '\0';
        offset += name_len;
    }
    techs->next_offset = offset;
    return 1;
fail:
    dat_free_techs(techs);
    return 0;
}
