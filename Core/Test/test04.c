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
#define TEST4_REPETITIONS 5
#else
#define TEST4_REPETITIONS GLOBAL_TEST_REPETITIONS
#endif

#define TEST4_END_VALUE 50

uint32_t fail_thread_id = 0;
uint32_t fail_counter = 0;

void my_assert(uint32_t condition, uint32_t thread_id)
{
	if (!condition)
	{
		fail_thread_id = thread_id;
		fail_counter++;
		assert(0);
	}
}



uint32_t test4_nr_tasks = 0;
volatile uint32_t test4_cm_memory = 0;
volatile uint32_t test4_task2_finished = 0;

typedef struct test4_args
{
	mutex_t* mutex_object;
	uint32_t value_to_deliver;
	uint32_t* finished;
	uint32_t thread_id;
} test4_args;

typedef struct test4_args2
{
	uint32_t* finished;
	uint32_t nr_threads;

} test4_args2;

void test4_task(void* args)
{
	test4_args* test4_args_object = (test4_args*) args;

	mutex_t* mutex_object = test4_args_object->mutex_object;
	uint32_t value_to_deliver = test4_args_object->value_to_deliver;
	uint32_t* finished = test4_args_object->finished;

	for (uint32_t i = 0; i < TEST4_END_VALUE; i++)
	{
		mutex_lock(mutex_object);
		my_assert(test4_cm_memory == 0, test4_args_object->thread_id);

		test4_cm_memory = value_to_deliver;

		yield();

		my_assert(test4_cm_memory == value_to_deliver, test4_args_object->thread_id);

		test4_cm_memory = 0;

		mutex_unlock(mutex_object);
		yield();
	}

	(*finished)++;
}

void test4_task2(void* args)
{
	test4_task2_finished = 0;

	volatile test4_args2* test4_args2_obj = (test4_args2*) args;

	while (1)
	{
		uint32_t threads_finished = 1;

		uint32_t i;
		for (i = 0; i < test4_args2_obj->nr_threads; i++)
		{
			threads_finished = test4_args2_obj->finished[i] && threads_finished;

			if (!threads_finished)
			{
				break;
			}
		}

		if (threads_finished)
		{
			break;
		}

		yield();
	}

	test4_task2_finished = 1;
}

uint32_t test4(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	fail_thread_id = 0;
	fail_counter = 0;

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
			args[j].thread_id = j;

			finished[j] = 0;
		}

		thread_attributes_t* threads_attributes = malloc(sizeof(*threads_attributes) * test4_nr_tasks);

		for (uint32_t j = 0; j < test4_nr_tasks; j++)
		{
			threads_attributes[j].function = test4_task;
			threads_attributes[j].function_arguments = &args[j];
			threads_attributes[j].stack_size = 2000;
			threads_attributes[j].thread_priority = rand() % 10 + 1;

			kernel_add_thread(kernel_object, &threads_attributes[j]);
		}

		test4_args2 test4_args2_obj = {
				.finished = finished,
				.nr_threads = test4_nr_tasks

		};
		thread_attributes_t thread2_attributes = {
				.function = test4_task2,
				.function_arguments = &test4_args2_obj,
				.stack_size = 1000,
				.thread_priority = 0
		};

		kernel_add_thread(kernel_object, &thread2_attributes);

		kernel_launch(kernel_object);

		kernel_destroy(kernel_object);


		for (uint32_t j = 0; j < test4_nr_tasks; j++)
		{
			assert(finished[j] == 1);
		}

		mutex_destroy(mutex_object);

		for (uint32_t j = 0; j < test4_nr_tasks; j++)
		{
			assert(finished[j] == 1);
		}


		free(args);
		free(threads_attributes);

		free(finished);

		assert(test4_task2_finished == 1);
	}


	return 0;
}
