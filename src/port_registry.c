/* The port's stand-in for the game's registry values. */
#include "port_registry.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void aoe_registry_load_ror(AoePortRegistryRor *r, const char *path) {
    FILE *f;
    char line[128];
    memset(r, 0, sizeof *r);
    snprintf(r->path, sizeof r->path, "%s", path);
    f = fopen(path, "r");
    if (!f) return;
    while (fgets(line, sizeof line, f) && r->count < AOE_REGISTRY_MAX) {
        char *eq = strchr(line, '=');
        size_t n;
        if (!eq) continue;
        n = (size_t)(eq - line);
        if (n == 0 || n >= sizeof r->names[0]) continue;
        memcpy(r->names[r->count], line, n);
        r->names[r->count][n] = '\0';
        r->values[r->count] = (int32_t)strtol(eq + 1, NULL, 10);
        r->count++;
    }
    fclose(f);
}

int32_t aoe_registry_get_ror(const AoePortRegistryRor *r, const char *name, int32_t fallback) {
    int32_t i;
    for (i = 0; i < r->count; ++i)
        if (strcmp(r->names[i], name) == 0) return r->values[i];
    return fallback;
}

int aoe_registry_set_ror(AoePortRegistryRor *r, const char *name, int32_t value) {
    FILE *f;
    int32_t i;
    for (i = 0; i < r->count; ++i)
        if (strcmp(r->names[i], name) == 0) break;
    if (i == r->count) {
        if (r->count >= AOE_REGISTRY_MAX || strlen(name) >= sizeof r->names[0]) return 0;
        snprintf(r->names[i], sizeof r->names[i], "%s", name);
        r->count++;
    }
    r->values[i] = value;
    f = fopen(r->path, "w");
    if (!f) return 0;
    for (i = 0; i < r->count; ++i) fprintf(f, "%s=%d\n", r->names[i], (int)r->values[i]);
    return fclose(f) == 0;
}
