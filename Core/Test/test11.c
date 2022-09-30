#include "tests.h"
#include <kernel/kernel.h>
#include <kernel/thread.h>
#include <kernel/atomic.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <kernel/mutex/mutex.h>

#ifndef GLOBAL_TEST_REPETITIONS
#define TEST11_REPETITIONS 5
#else
#define TEST11_REPETITIONS GLOBAL_TEST_REPETITIONS
#endif

volatile uint32_t test11_function1_counter = 0;
volatile uint32_t test11_function2_counter = 0;

void test11_function1(void* args);
void test11_function2(void* args);

void test11_function1(void* args)
{
	assert(test11_function2_counter == 0);

	delay(2000);
	assert(test11_function2_counter == 0);

	test11_function1_counter = 1;
}

void test11_function2(void* args)
{
	assert(test11_function1_counter == 0);

	while (!test11_function1_counter);

	test11_function2_counter = 1;
}

uint32_t test11(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	test11_function1_counter = 0;
	test11_function2_counter = 0;

	for (uint32_t i = 0; i < TEST11_REPETITIONS; i++)
	{
		test11_function1_counter = 0;
		test11_function2_counter = 0;

		kernel_attributes_t kernel_attributes_object = {
				.scheduler_algorithm = scheduler_algorithm
		};

		kernel_t* kernel = kernel_create(&kernel_attributes_object);

		thread_attributes_t func1 = {
					.function = test11_function1,
					.thread_name = "function1",
					.function_arguments = kernel,
					.stack_size = 1000,
					.thread_priority = 10
			};

		thread_attributes_t func2 = {
					.function = test11_function2,
					.thread_name = "function2",
					.function_arguments = 0,
					.stack_size = 1000,
					.thread_priority = 0
			};

		kernel_add_thread(kernel, &func1);
		kernel_add_thread(kernel, &func2);

		kernel_launch(kernel);

		kernel_destroy(kernel);

		assert(test11_function1_counter == 1 && test11_function2_counter == 1);
	}

	return 0;
}
