#ifndef VITA_AOE_PORT_REGISTRY_H
#define VITA_AOE_PORT_REGISTRY_H

#include <stdint.h>

/* The values the original keeps in the Windows registry
   (HKCU\Software\Microsoft\Games\Age of Empires\..., read and written by
   0x0046af50 / 0x0046ae20), kept by the port in a text file of
   "Name=value" lines (integers) in the data root. Integration support. */
enum { AOE_REGISTRY_MAX = 32 };
typedef struct {
    char path[600];
    int32_t count;
    char names[AOE_REGISTRY_MAX][48];
    int32_t values[AOE_REGISTRY_MAX];
} AoePortRegistryRor;

/* Loads `path` (missing: empty). */
void aoe_registry_load_ror(AoePortRegistryRor *r, const char *path);
/* The value, or `fallback` when absent (the original's -1 for a missing
   "Game File Number"). */
int32_t aoe_registry_get_ror(const AoePortRegistryRor *r, const char *name, int32_t fallback);
/* Sets and saves; 0 when the file cannot be written or the table is full. */
int aoe_registry_set_ror(AoePortRegistryRor *r, const char *name, int32_t value);

#endif
