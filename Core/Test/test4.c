/*
 * The purpose of this test is to verify if mutex works correctly (1 task / 3 tasks / 5 tasks, random order of adding tasks, random priorities of tasks)
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
#define TEST4_REPETITIONS 5
#else
#define TEST4_REPETITIONS GLOBAL_TEST_REPETITIONS
#endif

#define TEST4_END_VALUE 50

uint32_t test4_nr_tasks = 0;
volatile uint32_t test4_cm_memory = 0;
volatile uint32_t test4_task2_counter = 0;
volatile uint32_t test4_task2_finished = 0;

typedef struct test4_args
{
	mutex_t* mutex_object;
	uint32_t value_to_deliver;
	uint32_t* finished;
} test4_args;



void test4_task(void* args)
{
	test4_args* test4_args_object = (test4_args*) args;

	mutex_t* mutex_object = test4_args_object->mutex_object;
	uint32_t value_to_deliver = test4_args_object->value_to_deliver;
	uint32_t* finished = test4_args_object->finished;

	for (uint32_t i = 0; i < TEST4_END_VALUE; i++)
	{
		mutex_lock(mutex_object);
		assert(test4_cm_memory == 0);

		test4_cm_memory = value_to_deliver;

		YIELD();

		assert(test4_cm_memory == value_to_deliver);

		test4_cm_memory = 0;

		mutex_unlock(mutex_object);
		YIELD();
	}

	(*finished)++;
}

void test4_task2(void* args)
{
	test4_task2_finished = 0;
	test4_task2_counter = 0;

	for (uint32_t i = 0; i < test4_nr_tasks * TEST4_REPETITIONS; i++)
	{
		test4_task2_counter++;
		YIELD();
	}

	test4_task2_finished = 1;
}

uint32_t test4(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	for (uint32_t i = 0; i < TEST4_REPETITIONS; i++)
	{
		kernel_attributes_t kernel_attributes_object = {
				.scheduler_algorithm = scheduler_algorithm
		};

		kernel_t* kernel_object = kernel_create(&kernel_attributes_object);

		mutex_t* mutex_object = mutex_create();

		test4_nr_tasks = rand() % 10 + 1;

		test4_args* args = malloc(sizeof(*args) * test4_nr_tasks);
		uint32_t* finished = malloc(sizeof(*finished) * test4_nr_tasks);

		for (uint32_t j = 0; j < test4_nr_tasks; j++)
		{
			args[j].mutex_object = mutex_object;
			args[j].value_to_deliver = j + 1;
			args[j].finished = &finished[j];

			finished[j] = 0;
		}

		thread_attributes_t* threads_attributes = malloc(sizeof(*threads_attributes) * test4_nr_tasks);

		for (uint32_t j = 0; j < test4_nr_tasks; j++)
		{
			threads_attributes[j].function = test4_task;
			threads_attributes[j].function_arguments = &args[j];
			threads_attributes[j].stack_size = 2000;
			threads_attributes[j].thread_priority = rand() % 10;

			kernel_add_thread(kernel_object, &threads_attributes[j]);
		}

		thread_attributes_t thread2_attributes = {
				.function = test4_task2,
				.function_arguments = 0,
				.stack_size = 1000,
				.thread_priority = rand() % 16
		};

		kernel_add_thread(kernel_object, &thread2_attributes);

		kernel_launch(kernel_object);

		kernel_destroy(kernel_object);


		mutex_destroy(mutex_object);

		free(args);
		free(threads_attributes);

		for (uint32_t j = 0; j < test4_nr_tasks; j++)
		{
			assert(finished[j] == 1);
		}

		free(finished);

		assert(test4_task2_counter ==  test4_nr_tasks * TEST4_REPETITIONS);
		assert(test4_task2_finished == 1);
	}


	return 0;
}
