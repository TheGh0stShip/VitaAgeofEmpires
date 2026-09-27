#include "train_action.h"
#include <assert.h>

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
    return 0;
}
