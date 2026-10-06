#include "train_action.h"
#include <assert.h>
#include <string.h>

int main(void) {
    DatUnit unit = {0};
    AoeTrainAction action = {0};
    int16_t id = 0, percent = 0;
    unit.train_time = 10;
    action.unit_id = 83;
    action.trained_template = &unit;
    action.progress = 5.99f;
    action.state = 6;
    assert(aoe_train_action_progress_ror(&action, &id, &percent));
    assert(id == 83 && percent == 59); /* x87 helper truncates, never rounds. */
    action.progress = -0.19f;
    assert(aoe_train_action_progress_ror(&action, &id, &percent));
    assert(percent == -1);
    action.state = 1;
    action.trained_template = NULL;
    assert(aoe_train_action_progress_ror(&action, &id, &percent));
    assert(percent == 100);
    action.state = 6;
    assert(!aoe_train_action_progress_ror(&action, &id, &percent));
    assert(!aoe_train_action_progress_ror(NULL, &id, &percent));
    {   /* 0x004b54c0: readout with the name cut to the buffer */
        char name_text[] = "Legion";
        char buf[4] = {'x', 'x', 'x', 'x'};
        unit.name = name_text;
        action.trained_template = &unit;
        action.progress = 2.5f;
        assert(aoe_train_action_readout_ror(&action, &id, &percent, buf, 4));
        assert(id == 83 && percent == 25 && strcmp(buf, "Leg") == 0);
        assert(aoe_train_action_readout_ror(&action, &id, &percent, NULL, 0));
        assert(!aoe_train_action_readout_ror(&action, &id, &percent, buf, 0));
    }
    return 0;
}
