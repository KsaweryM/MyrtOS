/*
 * The purpose of this test is thread_yield works correctly
 */

#include "tests.h"
#include <kernel/kernel.h>
#include <kernel/thread.h>
#include <kernel/atomic.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <kernel/mutex/mutex.h>

#ifndef GLOBAL_TEST_REPETITIONS
#define TEST2_REPETITIONS 5
#else
#define TEST2_REPETITIONS GLOBAL_TEST_REPETITIONS
#endif

#define TEST1_END_VALUE 100

volatile int test2_counter = 0;
volatile int test2_d2 = 0;

void test2_task0(void* args)
{
	for (uint32_t i = 0; i < TEST1_END_VALUE; i++)
	{
		test2_counter++;
		yield();
		test2_d2 = test2_d2 | test2_counter;
	}
}


void test2_task1(void* args)
{
	for (uint32_t i = 0; i < TEST1_END_VALUE; i++)
	{
		test2_counter=0;
		yield();
	}
}

uint32_t test2(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	test2_counter = 0;
	test2_d2 = 0;

	thread_attributes_t task0_attributes = {
		.thread_name = "task1",
		.function = test2_task0,
		.function_arguments = (void*)0,
		.stack_size = 1000,
		.thread_priority = 0
	};

	thread_attributes_t task1_attributes = {
		.thread_name = "task1",
		.function = test2_task1,
		.function_arguments = (void*)0,
		.stack_size = 1000,
		.thread_priority = 0
	};


	kernel_attributes_t kernel_attributes_object = {
			.scheduler_algorithm = scheduler_algorithm
	};

	kernel_t* kernel_object = kernel_create(&kernel_attributes_object);

	kernel_add_thread(kernel_object, &task0_attributes);
	kernel_add_thread(kernel_object, &task1_attributes);

	kernel_launch(kernel_object);

	kernel_destroy(kernel_object);

	return 0;
}
