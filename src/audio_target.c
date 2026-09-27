#include "audio_target.h"

static AoeAudioTarget *find_tail(AoeAudioTarget *head) {
    AoeAudioTarget *slow, *fast, *tail;
    if (!head) return 0;
    slow = head;
    fast = head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) return 0;
    }
    tail = head;
    while (tail->next) tail = tail->next;
    return tail;
}

int aoe_audio_target_stop(AoeAudioTarget *head,
                          const AoeAudioTargetOps *ops) {
    AoeAudioTarget *tail;
    if (!ops || !ops->stop_voice) return 0;
    tail = find_tail(head);
    if (!tail) return 0;
    if (tail->name && tail->name[0] && tail->active && tail->voice)
        ops->stop_voice(ops->user, tail->voice);
    return 1;
}

int aoe_audio_target_expire(AoeAudioTarget *head,
                            const AoeAudioTargetOps *ops) {
    AoeAudioTarget *tail;
    if (!ops || !ops->stop_voice || !ops->release_voice ||
        !ops->free_buffer) return 0;
    tail = find_tail(head);
    if (!tail) return 0;
    if (tail->active) {
        if (!aoe_audio_target_stop(tail, ops)) return 0;
        if (tail->voice) {
            ops->release_voice(ops->user, tail->voice);
            tail->voice = 0;
        }
        if (tail->buffer_a) {
            ops->free_buffer(ops->user, tail->buffer_a);
            tail->buffer_a = 0;
        }
        if (tail->buffer_b) {
            ops->free_buffer(ops->user, tail->buffer_b);
            tail->buffer_b = 0;
        }
        tail->active = 0;
    }
    tail->pending = 0;
    return 1;
}

int aoe_audio_target_expire_callback(void *user, void *target) {
    return aoe_audio_target_expire(target, user);
}
