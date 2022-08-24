#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <stdint.h>
#include "thread.h"

typedef struct scheduler scheduler;

typedef struct scheduler_attributes
{
} scheduler_attributes;

scheduler* scheduler_create(const scheduler_attributes* scheduler_attributes_object);
void scheduler_destroy(scheduler* scheduler_object);

uint32_t scheduler_is_context_to_save(const scheduler* scheduler_object);
uint32_t scheduler_choose_next_thread(scheduler* scheduler_object, uint32_t SP_register);
void scheduler_add_thread(scheduler* scheduler_object, const thread_attributes* thread_attributes_object);

#endif
