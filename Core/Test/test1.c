/*
 * The purpose of this test is to verify if mutex works correctly (1 task / 3 tasks / 5 tasks, random order of adding tasks, random priorities of tasks)
 */

#include <kernel/kernel.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/atomic.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

#define TEST2_REPETITIONS 5
#define TEST2_END_VALUE 50

volatile uint32_t task1_counter = 0;
volatile uint32_t task2_counter = 0;

volatile uint32_t task1_finished = 0;
volatile uint32_t task2_finished = 0;
volatile uint32_t task3_finished = 0;

typedef struct task1_args
{
	mutex* mutex_object;
	uint32_t* counter;
	uint32_t* common;
} task1_args;

typedef struct task2_args
{
	mutex* mutex_object;
	uint32_t* counter;
	uint32_t* end;
} task2_args;

typedef struct task3_args
{
	uint32_t* end;
	uint32_t* common;
} task3_args;


void test1_task0(void* args)
{
	mutex* mutex_object = mutex_create();
	mutex_destroy(mutex_object);

	*((uint32_t*) args) = 1;
}

void test1_task1(void* args)
{
	CRITICAL_PATH_ENTER();

	task1_args* task1_args_object = (task1_args*) args;
	mutex_lock(task1_args_object->mutex_object);

	CRITICAL_PATH_EXIT();

	assert(task1_counter == 0);
	assert(task2_counter == 0);

	for (uint32_t i = 0; i < TEST2_END_VALUE; i++)
	{
		task1_counter++;

		assert(task1_counter == i + 1);
		assert(task2_counter == 0);

		thread_yield();
	}

	assert(task1_counter == TEST2_END_VALUE);
	assert(task2_counter == 0);

	mutex_unlock(task1_args_object->mutex_object);
	task1_finished++;
}

void test1_task2(void* args)
{
	task2_args* task2_args_object = (task2_args*) args;

	mutex_lock(task2_args_object->mutex_object);

	assert(task1_counter == TEST2_END_VALUE);
	assert(task2_counter == 0);

	for (uint32_t i = 0; i < TEST2_END_VALUE; i++)
	{
		task2_counter++;

		assert(task1_counter == TEST2_END_VALUE);
		assert(task2_counter == i + 1);

		thread_yield();
	}

	assert(task2_counter == TEST2_END_VALUE);
	assert(task1_counter == TEST2_END_VALUE);

	mutex_unlock(task2_args_object->mutex_object);
	task2_finished++;
}

void test1_task3(void* args)
{
	task3_args* task3_args_object = (task3_args*) args;

	for (uint32_t i = 0; i < 10 * TEST2_END_VALUE; i++)
	{
		thread_yield();
	}

	(*task3_args_object->end) = 1;
	task3_finished++;
}



uint32_t test1()
{
	for (uint32_t i = 0; i < TEST2_REPETITIONS; i++)
	{
		uint32_t end = 0;
		thread_attributes task0_attributes = {
			.thread_name = "task0",
			.function = test1_task0,
			.function_arguments = (void*)&end,
			.stack_size = 1000,
			.thread_priority = 0
		};


		kernel_attributes kernel_attributes_object = {

		};

		scheduler_attributes scheduler_attributes_object = {
			.algorithm = ROUND_ROBIN_SCHEDULING
		};

		kernel* kernel_object = kernel_create(&kernel_attributes_object, &scheduler_attributes_object);

		kernel_add_thread(kernel_object, &task0_attributes);
		kernel_launch(kernel_object);

		while (!end);

		kernel_destroy(kernel_object);
		// mutex constructor and destructor work properly

		task1_counter = 0;
		task2_counter = 0;

		kernel_object = kernel_create(&kernel_attributes_object, &scheduler_attributes_object);

		mutex* mutex_object = mutex_create();
		uint32_t counter = 0;
		uint32_t common = 0;
		end = 0;

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

		thread_attributes task1_attributes = {
					.thread_name = "task1",
					.function = test1_task1,
					.function_arguments = (void*)&task1_args_object,
					.stack_size = 1000,
					.thread_priority = rand() % 16
				};

		thread_attributes task2_attributes = {
					.thread_name = "task2",
					.function = test1_task2,
					.function_arguments = (void*)&task2_args_object,
					.stack_size = 1000,
					.thread_priority = rand() % 16
				};

		thread_attributes task3_attributes = {
					.thread_name = "task3",
					.function = test1_task3,
					.function_arguments = (void*)&task3_args_object,
					.stack_size = 1000,
					.thread_priority = rand() % 16
				};

		kernel_add_thread(kernel_object, &task1_attributes);
		kernel_add_thread(kernel_object, &task2_attributes);
		kernel_add_thread(kernel_object, &task3_attributes);

		kernel_launch(kernel_object);

		kernel_destroy(kernel_object);

		mutex_destroy(mutex_object);
	}

	assert(task1_finished == TEST2_REPETITIONS);
	assert(task2_finished == TEST2_REPETITIONS);
	assert(task3_finished == TEST2_REPETITIONS);

	return 0;
}
