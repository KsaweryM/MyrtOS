/*
 * The purpose of this test is to verify if mutex works correctly (1 task / 3 tasks / 5 tasks, random order of adding tasks, random priorities of tasks)
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
#define TEST1_REPETITIONS 5
#else
#define TEST1_REPETITIONS GLOBAL_TEST_REPETITIONS
#endif

#define TEST1_END_VALUE 50

volatile uint32_t task1_counter = 0;
volatile uint32_t task2_counter = 0;

volatile uint32_t task1_finished = 0;
volatile uint32_t task2_finished = 0;
volatile uint32_t task3_finished = 0;

typedef struct task1_args
{
	mutex_t* mutex_object;
	uint32_t* counter;
	uint32_t* common;
} task1_args;

typedef struct task2_args
{
	mutex_t* mutex_object;
	uint32_t* counter;
	uint32_t* end;
} task2_args;

typedef struct task3_args
{
	uint32_t* end;
	uint32_t* common;
} task3_args;

void test1_task1(void* args)
{
	task1_args* task1_args_object = (task1_args*) args;
	mutex_lock(task1_args_object->mutex_object);

	assert(task1_counter == 0);
	assert(task2_counter == 0);

	for (uint32_t i = 0; i < TEST1_END_VALUE; i++)
	{
		task1_counter++;

		assert(task1_counter == i + 1);
		assert(task2_counter == 0);

		yield();
	}

	assert(task1_counter == TEST1_END_VALUE);
	assert(task2_counter == 0);

	mutex_unlock(task1_args_object->mutex_object);
	task1_finished++;
}

void test1_task2(void* args)
{
	task2_args* task2_args_object = (task2_args*) args;

	mutex_lock(task2_args_object->mutex_object);

	assert(task1_counter == TEST1_END_VALUE);
	assert(task2_counter == 0);

	for (uint32_t i = 0; i < TEST1_END_VALUE; i++)
	{
		task2_counter++;

		assert(task1_counter == TEST1_END_VALUE);
		assert(task2_counter == i + 1);

		yield();
	}

	assert(task2_counter == TEST1_END_VALUE);
	assert(task1_counter == TEST1_END_VALUE);

	mutex_unlock(task2_args_object->mutex_object);
	task2_finished++;
}

void test1_task3(void* args)
{
	task3_args* task3_args_object = (task3_args*) args;

	for (uint32_t i = 0; i < 10 * TEST1_END_VALUE; i++)
	{
		yield();
	}

	(*task3_args_object->end) = 1;
	task3_finished++;
}



uint32_t test1(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	task1_counter = 0;
	task2_counter = 0;

	task1_finished = 0;
	task2_finished = 0;
	task3_finished = 0;

	for (uint32_t i = 0; i < TEST1_REPETITIONS; i++)
	{
		task1_counter = 0;
		task2_counter = 0;

		kernel_attributes_t kernel_attributes_object = {
						.scheduler_algorithm = scheduler_algorithm
				};

		kernel_t* kernel_object = kernel_create(&kernel_attributes_object);

		mutex_t* mutex_object = mutex_create();
		uint32_t counter = 0;
		uint32_t common = 0;
		uint32_t end = 0;

		task1_args task1_args_object = {
				.counter = &counter,
				.mutex_object = mutex_object,
				.common = &common
		};

		task2_args task2_args_object = {
				.counter = &counter,
				.mutex_object = mutex_object,
				.end = &end
		};

		task3_args task3_args_object = {
				.end = &end,
				.common = &common
		};

		thread_attributes_t task1_attributes = {
					.thread_name = "task1",
					.function = test1_task1,
					.function_arguments = (void*)&task1_args_object,
					.stack_size = 1000,
					.thread_priority = 11
				};

		thread_attributes_t task2_attributes = {
					.thread_name = "task2",
					.function = test1_task2,
					.function_arguments = (void*)&task2_args_object,
					.stack_size = 1000,
					.thread_priority = 10
				};

		thread_attributes_t task3_attributes = {
					.thread_name = "task3",
					.function = test1_task3,
					.function_arguments = (void*)&task3_args_object,
					.stack_size = 1000,
					.thread_priority = 10
				};


		if (scheduler_algorithm == PRIORITIZED_PREEMPTIVE_SCHEDULING_WITH_TIME_SLICING)
		{
			thread_attributes_t threads_attributes[] = {task1_attributes, task2_attributes, task3_attributes};
			uint32_t used_count = 0;
			uint32_t used[] = {0, 0, 0};

			while (used_count < 3)
			{
				uint32_t i = rand() % 3;

				if (!used[i])
				{
					kernel_add_thread(kernel_object, &threads_attributes[i]);
					used[i] = 1;
					used_count++;
				}
			}
		}
		else
		{
			kernel_add_thread(kernel_object, &task1_attributes);
			kernel_add_thread(kernel_object, &task2_attributes);
			kernel_add_thread(kernel_object, &task3_attributes);
		}

		kernel_launch(kernel_object);

		kernel_destroy(kernel_object);

		mutex_destroy(mutex_object);
	}

	assert(task1_finished == TEST1_REPETITIONS);
	assert(task2_finished == TEST1_REPETITIONS);
	assert(task3_finished == TEST1_REPETITIONS);

	return 0;
}
