#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <kernel/mutex/mutex.h>
#include <kernel/thread.h>
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
void scheduler_add_thread_control_block(scheduler_t* scheduler, thread_control_block_t* thread_control_block);
void scheduler_block_thread(scheduler_t* scheduler, uint32_t delay);
void scheduler_change_thread_priority(scheduler_t* scheduler, thread_control_block_t* thread_control_block, uint32_t priority);
uint32_t scheduler_get_nr_priorities(scheduler_t* scheduler);
thread_control_block_t* scheduler_get_current_thread(scheduler_t* scheduler);
void scheduler_set_mutex_state(scheduler_t* scheduler);
void scheduler_launch (scheduler_t* scheduler);

#endif
