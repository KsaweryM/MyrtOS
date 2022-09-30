#include "tests.h"
#include <kernel/kernel.h>
#include <kernel/thread.h>
#include <kernel/atomic.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <kernel/mutex/mutex.h>

#ifndef GLOBAL_TEST_REPETITIONS
#define TEST8_REPETITIONS 5
#else
#define TEST8_REPETITIONS GLOBAL_TEST_REPETITIONS
#endif

volatile uint32_t test8_function1_counter = 0;
volatile uint32_t test8_function2_counter = 0;

void test8_function1(void* args)
{
	assert(test8_function2_counter == 0);

	while (!test8_function2_counter);

	test8_function1_counter = 1;
}

void test8_function2(void* args)
{
	assert(test8_function1_counter == 0);

	test8_function2_counter = 1;
}

uint32_t test8(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	test8_function1_counter = 0;
	test8_function2_counter = 0;

	thread_attributes_t func1 = {
				.function = test8_function1,
				.thread_name = "function1",
				.function_arguments = 0,
				.stack_size = 1000,
				.thread_priority = 1
		};

	thread_attributes_t func2 = {
				.function = test8_function2,
				.thread_name = "function2",
				.function_arguments = 0,
				.stack_size = 1000,
				.thread_priority = 1
		};

	kernel_attributes_t kernel_attributes_object = {
			.scheduler_algorithm = scheduler_algorithm
	};

	kernel_t* kernel = kernel_create(&kernel_attributes_object);

	kernel_add_thread(kernel, &func1);
	kernel_add_thread(kernel, &func2);

	kernel_launch(kernel);

	kernel_destroy(kernel);

	assert(test8_function1_counter == 1 && test8_function2_counter == 1);

	return 0;
}
