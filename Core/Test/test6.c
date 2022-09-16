#include "tests.h"
#include <kernel/kernel.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/atomic.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

#ifndef GLOBAL_TEST_REPETITIONS
#define TEST6_REPETITIONS 5
#else
#define TEST6_REPETITIONS GLOBAL_TEST_REPETITIONS
#endif

#define TEST6_I 100


volatile uint32_t test6_basic_counter0 = 0;
volatile uint32_t test6_basic_counter1 = 0;
volatile uint32_t test6_basic_counter2 = 0;
volatile uint32_t test6_basic_counter3 = 0;
volatile uint32_t test6_basic_counter4 = 0;

volatile uint32_t test6_basic0_i, test6_basic1_i, test6_basic2_i, test6_basic3_i;

void test6_basic0(void* args)
{
	uint32_t delay_value = *(uint32_t*) args;

	for (test6_basic0_i = 0; test6_basic0_i <  TEST6_I; test6_basic0_i++)
	{
		delay(delay_value);
	}

	test6_basic_counter0++;
}

void test6_basic1(void* args)
{
	uint32_t delay_value = *(uint32_t*) args;

	for (test6_basic1_i = 0; test6_basic1_i <  TEST6_I; test6_basic1_i++)
	{
		delay(delay_value);
	}

	test6_basic_counter1++;
}

void test6_basic2(void* args)
{
	uint32_t delay_value = *(uint32_t*) args;

	for (test6_basic2_i = 0; test6_basic2_i <  TEST6_I; test6_basic2_i++)
	{
		delay(delay_value);
	}

	test6_basic_counter2++;
}

void test6_basic3(void* args)
{
	uint32_t delay_value = *(uint32_t*) args;

	for (test6_basic3_i = 0; test6_basic3_i <  TEST6_I; test6_basic3_i++)
	{
		delay(delay_value);
	}

	test6_basic_counter3++;
}

void test6_basic4(void* args)
{
	while (!(test6_basic_counter0 && test6_basic_counter1 && test6_basic_counter2 && test6_basic_counter3));

	test6_basic_counter4++;
}

uint32_t test6(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	for (uint32_t i = 0; i < TEST6_REPETITIONS; i++)
	{
		test6_basic_counter0 = 0;
		test6_basic_counter1 = 0;
		test6_basic_counter2 = 0;
		test6_basic_counter3 = 0;
		test6_basic_counter4 = 0;

		uint32_t delay0 = 99;
		uint32_t delay1 = rand() % 100 + 1;
		uint32_t delay2 = rand() % 100 + 1;
		uint32_t delay3 = rand() % 100 + 1;

		thread_attributes_t thread_basic0 = {
					.function = test6_basic0,
					.thread_name = "basic0",
					.function_arguments = &delay0,
					.stack_size = 1000,
					.thread_priority = 15
			};

		thread_attributes_t thread_basic1 = {
					.function = test6_basic1,
					.thread_name = "basic1",
					.function_arguments = &delay1,
					.stack_size = 1000,
					.thread_priority = 15
			};

		thread_attributes_t thread_basic2 = {
					.function = test6_basic2,
					.thread_name = "basic2",
					.function_arguments = &delay2,
					.stack_size = 1000,
					.thread_priority = 15
			};

		thread_attributes_t thread_basic3 = {
					.function = test6_basic3,
					.thread_name = "basic3",
					.function_arguments = &delay3,
					.stack_size = 1000,
					.thread_priority = 15
			};

		thread_attributes_t thread_basic4 = {
					.function = test6_basic4,
					.thread_name = "basic4",
					.function_arguments = 0,
					.stack_size = 1000,
					.thread_priority = 15
			};

		kernel_attributes_t kernel_attributes_object = {
				.scheduler_algorithm = scheduler_algorithm
		};

		kernel_t* kernel_object = kernel_create(&kernel_attributes_object);

		kernel_add_thread(kernel_object, &thread_basic0);
		kernel_add_thread(kernel_object, &thread_basic1);
		kernel_add_thread(kernel_object, &thread_basic2);
		kernel_add_thread(kernel_object, &thread_basic3);
		kernel_add_thread(kernel_object, &thread_basic4);

		kernel_launch(kernel_object);

		kernel_destroy(kernel_object);

		assert(test6_basic_counter0 && test6_basic_counter1 && test6_basic_counter2 && test6_basic_counter3 && test6_basic_counter4);
	}

	return 0;
}
