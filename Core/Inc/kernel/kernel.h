#ifndef __KERNEL_H
#define __KERNEL_H

#include <kernel/mutex/mutex.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/thread.h>
#include <stdint.h>
#include <stm32l4xx.h>

typedef struct kernel_t kernel_t;

typedef struct kernel_attributes_t
{
	SCHEDULER_ALGORITHM scheduler_algorithm;
} kernel_attributes_t;

kernel_t* kernel_get_instance(void);
kernel_t* kernel_create(const kernel_attributes_t* kernel_attributes);
void kernel_destroy(kernel_t* kernel);

void kernel_launch(const kernel_t* kernel);

void kernel_add_thread(kernel_t* kernel, const thread_attributes_t* thread_attributes);
mutex_t* kernel_create_mutex(kernel_t* kernel);

void delay(uint32_t delay);
uint32_t get_CPU_frequency();

void kernel_change_context_switch_period_in_milliseconds(kernel_t* kernel, uint32_t period);

#endif
