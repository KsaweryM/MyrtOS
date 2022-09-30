#include "tests.h"
#include <kernel/kernel.h>
#include <kernel/thread.h>
#include <kernel/atomic.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <kernel/mutex/mutex.h>

#ifndef GLOBAL_TEST_REPETITIONS
#define TEST10_REPETITIONS 5
#else
#define TEST10_REPETITIONS GLOBAL_TEST_REPETITIONS
#endif

volatile uint32_t test10_function1_counter = 0;
volatile uint32_t test10_function2_counter = 0;

void test10_function1(void* args);
void test10_function2(void* args);

void test10_function1(void* args)
{
	thread_attributes_t func2 = {
				.function = test10_function2,
				.thread_name = "function2",
				.function_arguments = 0,
				.stack_size = 1000,
				.thread_priority = 1
		};

	kernel_t* kernel = args;

	kernel_add_thread(kernel, &func2);

	assert(test10_function2_counter == 1);

	test10_function1_counter = 1;
}

void test10_function2(void* args)
{
	assert(test10_function1_counter == 0);
	test10_function2_counter = 1;
}

uint32_t test10(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	test10_function1_counter = 0;
	test10_function2_counter = 0;

	for (uint32_t i = 0; i < TEST10_REPETITIONS; i++)
	{
		test10_function1_counter = 0;
		test10_function2_counter = 0;

		kernel_attributes_t kernel_attributes = {
				.scheduler_algorithm = scheduler_algorithm
		};

		kernel_t* kernel = kernel_create(&kernel_attributes);

		thread_attributes_t func1 = {
					.function = test10_function1,
					.thread_name = "function1",
					.function_arguments = kernel,
					.stack_size = 1000,
					.thread_priority = 0
			};

		kernel_add_thread(kernel, &func1);

		kernel_launch(kernel);

		kernel_destroy(kernel);

		assert(test10_function1_counter == 1 && test10_function2_counter == 1);
	}

	return 0;
}
