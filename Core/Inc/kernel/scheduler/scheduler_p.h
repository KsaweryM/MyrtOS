#ifndef __SCHEDULER_P_H
#define __SCHEDULER_P_H

#include <kernel/thread.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/mutex.h>
#include <stdint.h>

typedef void (*scheduler_destroy_t) (scheduler_t* scheduler);
typedef void (*scheduler_add_thread_t) (scheduler_t* scheduler, const thread_attributes_t* thread_attributes);
typedef uint32_t* (*scheduler_choose_next_thread_t) (scheduler_t* scheduler, uint32_t* SP_register);
typedef mutex_t* (*scheduler_create_mutex_t) (scheduler_t* scheduler);

struct scheduler_t
{
	scheduler_destroy_t scheduler_destroy;
	scheduler_add_thread_t scheduler_add_thread;
	scheduler_choose_next_thread_t scheduler_choose_next_thread;
	scheduler_create_mutex_t scheduler_create_mutex;
};

#endif
