#ifndef __KERNEL_H
#define __KERNEL_H

#include "stm32l4xx.h"
#include "thread.h"
#include <stdint.h>

void kernel_create(void);
void kernel_destroy(void);

void kernel_launch(void);
void kernel_suspend(void);
void kernel_resume(void);

void kernel_add_thread(const thread_attributes* thread_attributes_object);

void thread_delay(uint32_t seconds);
void thread_yield(void);

#endif
