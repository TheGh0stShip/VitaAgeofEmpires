#include "live_unit_registry.h"
#include <assert.h>
#include <limits.h>
#include <stddef.h>
int main(void) {
    int p0=10,p2=12,n1=21,n3=23;
    void *positive[]={&p0,NULL,&p2};
    void *negative[]={NULL,&n1,NULL,&n3};
    AoeLiveUnitRegistry r={positive,3,negative,4};
    assert(aoe_live_unit_registry_lookup(&r,0)==&p0);
    assert(!aoe_live_unit_registry_lookup(&r,1));
    assert(aoe_live_unit_registry_lookup(&r,2)==&p2);
    assert(!aoe_live_unit_registry_lookup(&r,3));
    assert(aoe_live_unit_registry_lookup(&r,-1)==&n1);
    assert(!aoe_live_unit_registry_lookup(&r,-2));
    assert(aoe_live_unit_registry_lookup(&r,-3)==&n3);
    assert(!aoe_live_unit_registry_lookup(&r,-4));
    assert(!aoe_live_unit_registry_lookup(&r,INT32_MIN));
    assert(!aoe_live_unit_registry_lookup(NULL,0));
    r.positive_count=-1;assert(!aoe_live_unit_registry_lookup(&r,0));
    return 0;
}
