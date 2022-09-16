#include "tests.h"
#include <kernel/kernel.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/atomic.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

#ifndef GLOBAL_TEST_REPETITIONS
#define TEST5_REPETITIONS 5
#else
#define TEST5_REPETITIONS GLOBAL_TEST_REPETITIONS
#endif

#define TEST5_I 10

#define DELAY0 100
#define DELAY1 1000
#define DELAY2 500
#define DELAY3 200


volatile uint32_t test5_basic_counter0 = 0;
volatile uint32_t test5_basic_counter1 = 0;
volatile uint32_t test5_basic_counter2 = 0;
volatile uint32_t test5_basic_counter3 = 0;
volatile uint32_t test5_basic_counter4 = 0;

volatile uint32_t test5_basic0_i, test5_basic1_i, test5_basic2_i, test5_basic3_i;

void test5_basic0(void* args)
{
	for (test5_basic0_i = 0; test5_basic0_i <  TEST5_I; test5_basic0_i++)
	{
		delay(DELAY0);
	}

	test5_basic_counter0++;
}

void test5_basic1(void* args)
{
	for (test5_basic1_i = 0; test5_basic1_i <  TEST5_I; test5_basic1_i++)
	{
		delay(DELAY1);
	}

	test5_basic_counter1++;
}

void test5_basic2(void* args)
{
	for (test5_basic2_i = 0; test5_basic2_i <  TEST5_I; test5_basic2_i++)
	{
		delay(DELAY2);
	}

	test5_basic_counter2++;
}

void test5_basic3(void* args)
{
	for (test5_basic3_i = 0; test5_basic3_i <  TEST5_I; test5_basic3_i++)
	{
		delay(DELAY3);
	}

	test5_basic_counter3++;
}

void test5_basic4(void* args)
{
	while (!(test5_basic_counter0 && test5_basic_counter1 && test5_basic_counter2 && test5_basic_counter3));

	test5_basic_counter4++;
}

uint32_t test5(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	thread_attributes_t thread_basic0 = {
				.function = test5_basic0,
				.thread_name = "basic0",
				.function_arguments = 0,
				.stack_size = 1000,
				.thread_priority = 15
		};

	thread_attributes_t thread_basic1 = {
				.function = test5_basic1,
				.thread_name = "basic1",
				.function_arguments = 0,
				.stack_size = 1000,
				.thread_priority = 15
		};

	thread_attributes_t thread_basic2 = {
				.function = test5_basic2,
				.thread_name = "basic2",
				.function_arguments = 0,
				.stack_size = 1000,
				.thread_priority = 15
		};

	thread_attributes_t thread_basic3 = {
				.function = test5_basic3,
				.thread_name = "basic3",
				.function_arguments = 0,
				.stack_size = 1000,
				.thread_priority = 15
		};

	thread_attributes_t thread_basic4 = {
				.function = test5_basic4,
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

	assert(test5_basic_counter0 && test5_basic_counter1 && test5_basic_counter2 && test5_basic_counter3 && test5_basic_counter4);

	return 0;
}
