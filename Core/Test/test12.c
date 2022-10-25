#include "tests.h"
#include <kernel/kernel.h>
#include <kernel/thread.h>
#include <kernel/atomic.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <kernel/mutex/mutex.h>

volatile uint32_t change_contex = 1;
volatile int test_12_counter = 0;

void test12_function1(void* args)
{
	while (1)
	{
		test_12_counter++;
		change_contex = 1;
		while (change_contex);
	}
}

void test12_function2(void* args)
{
	while (1)
	{
		change_contex = 0;
		yield();
	}
}

uint32_t test12(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	thread_attributes_t timer0_attributes = {
		.function = test12_function1,
		.function_arguments = 0,
		.stack_size = 1000,
		.thread_priority = 0
	};

	thread_attributes_t timer1_attributes = {
		.function = test12_function2,
		.function_arguments = 0,
		.stack_size = 1000,
		.thread_priority = 0
	};

	kernel_attributes_t kernel_attributes_object = {
			.scheduler_algorithm = scheduler_algorithm
	};

	kernel_t* kernel_object = kernel_create(&kernel_attributes_object);

	kernel_add_thread(kernel_object, &timer0_attributes);
	kernel_add_thread(kernel_object, &timer1_attributes);

	kernel_change_context_switch_period_in_milliseconds(kernel_object, 4000);

	kernel_launch(kernel_object);

	kernel_destroy(kernel_object);

	return 0;
}
