/*
 * The purpose of this test is thread_yield works correctly
 */

#include "tests.h"
#include <kernel/kernel.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/atomic.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

#ifndef GLOBAL_TEST_REPETITIONS
#define TEST3_REPETITIONS 5
#else
#define TEST3_REPETITIONS GLOBAL_TEST_REPETITIONS
#endif

#define TEST3_END_VALUE 100

volatile uint32_t test3_task0_finished = 0;
volatile uint32_t test3_task1_finished = 0;
volatile uint32_t test3_task2_finished = 0;
volatile uint32_t test3_task3_finished = 0;

volatile int test3_task0_counter = 0;
volatile int test3_task1_counter = 0;
volatile int test3_task2_counter = 0;
volatile int test3_task3_counter = 0;

void test3_task0(void* args)
{
	for (uint32_t i = 0; i < TEST3_END_VALUE; i++)
	{
		test3_task0_counter++;
		assert(test3_task0_counter == test3_task1_counter + 1);
		assert(test3_task0_counter == test3_task2_counter + 1);
		assert(test3_task0_counter == test3_task3_counter + 1);

		YIELD();
	}

	test3_task0_finished++;
}

void test3_task1(void* args)
{
	for (uint32_t i = 0; i < TEST3_END_VALUE; i++)
	{
		test3_task1_counter++;
		assert(test3_task1_counter == test3_task0_counter);
		assert(test3_task1_counter == test3_task2_counter + 1);
		assert(test3_task1_counter == test3_task3_counter + 1);

		YIELD();
	}

	test3_task1_finished++;
}

void test3_task2(void* args)
{
	for (uint32_t i = 0; i < TEST3_END_VALUE; i++)
	{
		test3_task2_counter++;
		assert(test3_task2_counter == test3_task0_counter);
		assert(test3_task2_counter == test3_task1_counter);
		assert(test3_task2_counter == test3_task3_counter + 1);

		YIELD();
	}

	test3_task2_finished++;
}

void test3_task3(void* args)
{
	for (uint32_t i = 0; i < TEST3_END_VALUE; i++)
	{
		test3_task3_counter++;
		assert(test3_task3_counter == test3_task0_counter);
		assert(test3_task3_counter == test3_task1_counter);
		assert(test3_task3_counter == test3_task2_counter);

		YIELD();
	}

	test3_task3_finished++;
}

uint32_t test3(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	for (uint32_t i = 0; i < TEST3_REPETITIONS; i++)
	{
		thread_attributes_t task0_attributes = {
			.thread_name = "task0",
			.function = test3_task0,
			.function_arguments = (void*)0,
			.stack_size = 1000,
			.thread_priority = 0
		};

		thread_attributes_t task1_attributes = {
			.thread_name = "task1",
			.function = test3_task1,
			.function_arguments = (void*)0,
			.stack_size = 1000,
			.thread_priority = 0
		};

		thread_attributes_t task2_attributes = {
			.thread_name = "task2",
			.function = test3_task2,
			.function_arguments = (void*)0,
			.stack_size = 1000,
			.thread_priority = 0
		};

		thread_attributes_t task3_attributes = {
			.thread_name = "task3",
			.function = test3_task3,
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
		kernel_add_thread(kernel_object, &task2_attributes);
		kernel_add_thread(kernel_object, &task3_attributes);

		kernel_launch(kernel_object);

		kernel_destroy(kernel_object);
	}

	assert(test3_task0_finished == TEST3_REPETITIONS);
	assert(test3_task1_finished == TEST3_REPETITIONS);
	assert(test3_task2_finished == TEST3_REPETITIONS);
	assert(test3_task3_finished == TEST3_REPETITIONS);

	return 0;
}

