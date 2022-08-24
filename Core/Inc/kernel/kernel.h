#ifndef __KERNEL_H
#define __KERNEL_H

#include <kernel/scheduler/scheduler.h>
#include <kernel/thread.h>
#include "stm32l4xx.h"
#include <stdint.h>

typedef struct kernel kernel;

typedef struct kernel_attributes
{

} kernel_attributes;

kernel* kernel_create(const kernel_attributes* kernel_attributes_object, const scheduler_attributes* scheduler_attributes_object);
void kernel_destroy(kernel* kernel_object);

void kernel_launch(const kernel* kernel_object);
void kernel_suspend(const kernel* kernel_object);
void kernel_resume(const kernel* kernel_object);

void kernel_add_thread(kernel* kernel_object, const thread_attributes* thread_attributes_object);

void thread_delay(uint32_t seconds);
void thread_yield(void);

#endif
