#ifndef VITA_AOE_WORLD_ROR_SCENARIO_H
#define VITA_AOE_WORLD_ROR_SCENARIO_H

#include <stdint.h>

#include "aoe_stream.h"
#include "scenario_info.h"
#include "scenario_object.h"

/* Patched Rise of Rome 1.0a: the RoR scenario object's binary constructor
   0x00506900(stream, world) (0x514c bytes, vtable 0x0054a3c8; the RoR
   world's scenario factory, through the tribe game's 0x500270, which
   passes world 0). The base scenario object (AoeScenarioObjectRor, read
   by 0x474d40) is followed in the original allocation by the RoR
   scenario-info fields (AoeScenarioInfoRor); both carry their original
   offsets. Every gate compares the base's version, the f32 at +0x1888,
   with an x87 float compare. In order:
   - goals reset (0x00506f40), diplomacy 3 (0x00506fb0), every tech
     enabled (0x00506e40); victory type +0x5140 = 0, score +0x5144 = 900,
     time limit +0x5148 = 9000, all-required +0x4be8 = 0, the selected
     slot +0x4bf0 = 0;
   - version not above 1.13 (0x00545e80): the 16 0x100-byte player names
     (+0x0d) and per slot value_194c (+0x194c), the four start resources
     (+0x1990), value_18cc (+0x18cc), value_190c (+0x190c) and the player
     kind (+0x188c); above 1.13 only the start resources;
   - above 1.02 (0x00545e4c) a discarded marker dword (-99 when saved);
   - the six goals (+0x1a90, 0x18 bytes) and all-required (+0x4be8);
   - at least 1.13: victory type, score and time limit; below (or NaN):
     4 (custom), 900 and 9000;
   - the diplomacy (+0x47a8, 0x400) and the 16 slots' 12 condition records
     (+0x1aa8, 0x2d0 each);
   - above 1.02 a marker; exactly 1.01 (0x3f8147ae) sixteen 0x40-byte
     reads into the allied victory (+0x4ba8, the last one kept); at least
     1.02 one 0x40-byte allied victory;
   - above 1.03 (0x00545e60) the tech toggles (+0x4bf4, 0x500);
   - above 1.04 (0x0054a3e0): at least 1.12 (0x0054a3dc) the three
     options (+0x50f4), else option 0 read and options 1 and 2 cleared;
   - above 1.05 (0x0054a3d8) the starting ages (+0x5100, 0x40);
   - above 1.02 a marker;
   - custom conquest (+0x1a90) = the conquest byte +0x0c.
   Fields the version skips keep their prior contents (in the original,
   operator new's: the allied victory below 1.01, the options up to 1.04
   and the starting ages up to 1.05). The +0x0c byte is modelled twice
   (obj->field_c and info->conquest); the read copies the former into
   the latter. All values are little-endian. */
typedef struct {
    void *context;
    /* 0x474d40(stream, world): the base scenario object read (vtable
       0x00545e3c), starting with its version (+0x1888); fills `obj`
       (+0x0c among it) and returns that version. Required. */
    float (*base_read)(void *context, AoeOutcomeStream *s, void *world,
                       AoeScenarioObjectRor *obj);
} AoeRorScenarioReadOpsRor;

/* Returns the version the base read, or 0.0 (nothing read) when an
   argument or ops->base_read is missing. */
float aoe_ror_scenario_read_ror(AoeScenarioInfoRor *info, AoeScenarioObjectRor *obj,
                                AoeOutcomeStream *s, void *world,
                                const AoeRorScenarioReadOpsRor *ops);

#endif
