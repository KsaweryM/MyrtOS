#ifndef __SCHEDULER_P_H
#define __SCHEDULER_P_H

#include <kernel/mutex/mutex.h>
#include <kernel/thread.h>
#include <kernel/scheduler/scheduler.h>
#include <stdint.h>

typedef void (*scheduler_destroy_t) (scheduler_t* scheduler);
typedef uint32_t* (*scheduler_choose_next_thread_t) (scheduler_t* scheduler, uint32_t* SP_register);
typedef void (*scheduler_add_thread_t) (scheduler_t* scheduler, const thread_attributes_t* thread_attributes);
typedef void (*scheduler_add_thread_control_block_t) (scheduler_t* scheduler, thread_control_block_t* thread_control_block);
typedef void (*scheduler_block_thread_t) (scheduler_t* scheduler, uint32_t delay);
typedef void (*scheduler_change_thread_priority_t) (scheduler_t* scheduler, thread_control_block_t* thread_control_block, uint32_t priority);
typedef uint32_t (*scheduler_get_nr_priorities_t) (scheduler_t* scheduler);
typedef void (*scheduler_set_mutex_t) (scheduler_t* scheduler, mutex_t* mutex);
typedef thread_control_block_t* (*scheduler_get_current_thread_t) (scheduler_t* scheduler);
typedef void (*scheduler_set_mutex_state_t) (scheduler_t* scheduler);

struct scheduler_t
{
	scheduler_destroy_t scheduler_destroy;
	scheduler_choose_next_thread_t scheduler_choose_next_thread;
	scheduler_add_thread_t scheduler_add_thread;
	scheduler_add_thread_control_block_t scheduler_add_thread_control_block;
	scheduler_block_thread_t scheduler_block_thread;
	scheduler_change_thread_priority_t scheduler_change_thread_priority;
	scheduler_get_nr_priorities_t scheduler_get_nr_priorities;
	scheduler_get_current_thread_t scheduler_get_current_thread;
	scheduler_set_mutex_state_t scheduler_set_mutex_state;
};

#endif
