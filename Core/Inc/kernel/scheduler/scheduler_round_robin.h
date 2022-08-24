#ifndef __SCHEDULER_ROUND_ROBIN_H
#define __SCHEDULER_ROUND_ROBIN_H

#include "kernel/scheduler/scheduler.h"

scheduler* scheduler_round_robin_create(const scheduler_attributes* scheduler_attributes_object);
void scheduler_round_robin_destroy(scheduler* scheduler_object);

uint32_t scheduler_round_robin_is_context_to_save(const scheduler* scheduler_object);
uint32_t scheduler_round_robin_choose_next_thread(scheduler* scheduler_object, uint32_t SP_register);
void scheduler_round_robin_add_thread(scheduler* scheduler_object, const thread_attributes* thread_attributes_object);

#endif
