#include "port_registry.h"

#include <assert.h>
#include <stdio.h>

int main(int argc, char **argv) {
    AoePortRegistryRor r;
    const char *path = argc > 1 ? argv[1] : "port_registry_test.txt";
    remove(path);
    aoe_registry_load_ror(&r, path);
    assert(aoe_registry_get_ror(&r, "Game File Number", -1) == -1);
    assert(aoe_registry_set_ror(&r, "Game File Number", 2) && aoe_registry_set_ror(&r, "Scroll Speed", 84));
    assert(aoe_registry_set_ror(&r, "Game File Number", 3));
    aoe_registry_load_ror(&r, path);
    assert(r.count == 2 && aoe_registry_get_ror(&r, "Game File Number", -1) == 3 &&
           aoe_registry_get_ror(&r, "Scroll Speed", 0) == 84);
    remove(path);
    puts("registry ok");
    return 0;
}
