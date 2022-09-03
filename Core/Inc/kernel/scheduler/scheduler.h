#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <stdint.h>

typedef struct scheduler_t scheduler_t;

typedef enum SCHEDULER_ALGORITHM
{
	ROUND_ROBIN_SCHEDULING,
	PRIORITIZED_PREEMPTIVE_SCHEDULING_WITH_TIME_SLICING,
	PRIORITIZED_PREEMPTIVE_SCHEDULING_WITHOUT_TIME_SLICING,
	COOPERATIVE_SCHEDULING
} SCHEDULER_ALGORITHM;

void scheduler_destroy(scheduler_t* scheduler);
uint32_t* scheduler_choose_next_thread(scheduler_t* scheduler, uint32_t* SP_register);
void scheduler_add_thread(scheduler_t* scheduler, const thread_attributes_t* thread_attributes);
mutex_t* scheduler_create_mutex(scheduler_t* scheduler);

#endif
