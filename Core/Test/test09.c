#include "tests.h"
#include <kernel/kernel.h>
#include <kernel/thread.h>
#include <kernel/atomic.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <kernel/mutex/mutex.h>

#ifndef GLOBAL_TEST_REPETITIONS
#define TEST9_REPETITIONS 5
#else
#define TEST9_REPETITIONS GLOBAL_TEST_REPETITIONS
#endif


volatile uint32_t test9_function1_counter = 0;
volatile uint32_t test9_function2_counter = 0;
volatile int test9_counter = 0;

void test9_function1(void* args)
{
	assert(test9_function2_counter == 0);

	test9_counter = 0;
	while (!test9_function2_counter)
	{
		test9_counter++;

		assert(test9_function2_counter == 0);

		if (test9_counter == 100000)
		{
			yield();
		}
	}

	test9_function1_counter = 1;
}

void test9_function2(void* args)
{
	assert(test9_function1_counter == 0);

	test9_function2_counter = 1;
}

uint32_t test9(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	test9_function1_counter = 0;
	test9_function2_counter = 0;
	test9_counter = 0;

	thread_attributes_t func1 = {
				.function = test9_function1,
				.thread_name = "function1",
				.function_arguments = 0,
				.stack_size = 1000,
				.thread_priority = 1
		};

	thread_attributes_t func2 = {
				.function = test9_function2,
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

	assert(test9_function1_counter == 1 && test9_function2_counter == 1);

	return 0;
}
