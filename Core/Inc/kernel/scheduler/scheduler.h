#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <kernel/thread.h>
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
};

typedef enum scheduler_algorithm
{
	round_robin_scheduling,
	prioritized_preemptive_scheduling_with_time_slicing,
	prioritized_preemptive_scheduling_without_time_slicing,
	cooperative_scheduling
} scheduler_algorithm;

typedef struct scheduler_attributes
{
  scheduler_algorithm algorithm;
} scheduler_attributes;

scheduler* scheduler_create(const scheduler_attributes* scheduler_attributes_object);
void scheduler_destroy(scheduler* scheduler_object);

uint32_t scheduler_is_context_to_save(const scheduler* scheduler_object);
uint32_t scheduler_choose_next_thread(scheduler* scheduler_object, uint32_t SP_register);
void scheduler_add_thread(scheduler* scheduler_object, const thread_attributes* thread_attributes_object);

#endif
