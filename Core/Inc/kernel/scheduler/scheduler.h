#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <stdint.h>

typedef struct scheduler scheduler;
typedef struct scheduler_methods scheduler_methods;

struct scheduler
{
  void* scheduler_data;
  scheduler_methods* scheduler_methods;
};

struct scheduler_methods
{
  void (*scheduler_destroy) (scheduler* scheduler_object);
  uint32_t (*scheduler_is_context_to_save) (const scheduler* scheduler_object);
  void (*scheduler_add_thread) (scheduler* scheduler_object, const thread_attributes* thread_attributes_object);
  uint32_t (*scheduler_choose_next_thread) (scheduler* scheduler_object, uint32_t SP_register);
  mutex* (*scheduler_create_mutex) (scheduler* scheduler_object);
};

typedef enum SCHEDULER_ALGORITHM
{
	ROUND_ROBIN_SCHEDULING,
	PRIORITIZED_PREEMPTIVE_SCHEDULING_WITH_TIME_SLICING,
	PRIORITIZED_PREEMPTIVE_SCHEDULING_WITHOUT_TIME_SLICING,
	COOPERATIVE_SCHEDULING
} SCHEDULER_ALGORITHM;

typedef struct scheduler_attributes
{
	SCHEDULER_ALGORITHM algorithm;
} scheduler_attributes;

scheduler* scheduler_create(const scheduler_attributes* scheduler_attributes_object);
void scheduler_destroy(scheduler* scheduler_object);

uint32_t scheduler_is_context_to_save(const scheduler* scheduler_object);
uint32_t scheduler_choose_next_thread(scheduler* scheduler_object, uint32_t SP_register);
void scheduler_add_thread(scheduler* scheduler_object, const thread_attributes* thread_attributes_object);
mutex* scheduler_create_mutex(scheduler* scheduler_object);

#endif
