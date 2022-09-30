#include "tests.h"
#include <kernel/kernel.h>
#include <kernel/thread.h>
#include <kernel/atomic.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <kernel/mutex/mutex.h>

#ifndef GLOBAL_TEST_REPETITIONS
#define TEST7_REPETITIONS 5
#else
#define TEST7_REPETITIONS GLOBAL_TEST_REPETITIONS
#endif

volatile uint32_t test7_lowest_priority_counter = 0;
volatile uint32_t test7_average_priority_counter = 0;
volatile uint32_t test7_highest_priority_counter = 0;

typedef struct test7_args_t
{
	mutex_t* mutex;
	kernel_t* kernel;
	thread_attributes_t* test7_medium_priority_attributes;
	thread_attributes_t* test7_highest_priority_attributes;
} test7_args_t;

void test7_lowest_priority(void* args);
void test7_medium_priority(void* args);
void test7_highest_priority(void* args);

void test7_lowest_priority(void* args)
{
	test7_lowest_priority_counter = 0;

	test7_args_t* test7_args = (test7_args_t*) args;

	mutex_lock(test7_args->mutex);

	kernel_add_thread(test7_args->kernel, test7_args->test7_highest_priority_attributes);
	yield();
	// test7_average_priority_counter == 0 because test7_medium_priority thread wasn't added to kernel
	// test7_highest_priority_counter == 0 because test7_highest_priority was locked on mutex
	assert(test7_average_priority_counter == 0 && test7_highest_priority_counter == 0);

	kernel_add_thread(test7_args->kernel, test7_args->test7_medium_priority_attributes);
	yield();
	// test7_average_priority_counter == 0 because test7_lowest_priority inherited highest priority and it has higher priority than test7_medium_priority
	// test7_highest_priority_counter == 0 because test7_highest_priority was locked on mutex
	assert(test7_average_priority_counter == 0 && test7_highest_priority_counter == 0);

	mutex_unlock(test7_args->mutex);
	yield();
	// test7_lowest_priority has lowest priority again
	assert(test7_average_priority_counter == 1 && test7_highest_priority_counter == 1);

	test7_lowest_priority_counter = 1;
}

void test7_medium_priority(void* args)
{
	assert(test7_lowest_priority_counter == 0 && test7_highest_priority_counter == 1);
	test7_average_priority_counter = 1;
}

void test7_highest_priority(void* args)
{
	test7_highest_priority_counter = 0;

	test7_args_t* test7_args = (test7_args_t*) args;


	mutex_lock(test7_args->mutex);
	assert(test7_lowest_priority_counter == 0 && test7_average_priority_counter == 0);
	yield();

	mutex_unlock(test7_args->mutex);
	assert(test7_lowest_priority_counter == 0 && test7_average_priority_counter == 0);
	yield();

	test7_highest_priority_counter = 1;
}

uint32_t test7(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	test7_lowest_priority_counter = 0;
	test7_average_priority_counter = 0;
	test7_highest_priority_counter = 0;

	kernel_attributes_t kernel_attributes_object = {
			.scheduler_algorithm = scheduler_algorithm
	};

	kernel_t* kernel = kernel_create(&kernel_attributes_object);

	test7_args_t test7_args;

	test7_args.kernel = kernel;
	test7_args.mutex = mutex_create();;

	thread_attributes_t lowest_attributes = {
				.function = test7_lowest_priority,
				.thread_name = "lowest",
				.function_arguments = &test7_args,
				.stack_size = 1000,
				.thread_priority = 0
		};

	thread_attributes_t medium_attributes = {
				.function = test7_medium_priority,
				.thread_name = "medium",
				.function_arguments = &test7_args,
				.stack_size = 1000,
				.thread_priority = 1
		};

	thread_attributes_t highest_attributes = {
				.function = test7_highest_priority,
				.thread_name = "highest",
				.function_arguments = &test7_args,
				.stack_size = 1000,
				.thread_priority = 2
		};

	test7_args.test7_medium_priority_attributes = &medium_attributes;
	test7_args.test7_highest_priority_attributes = &highest_attributes;

	kernel_add_thread(kernel, &lowest_attributes);

	kernel_launch(kernel);

	kernel_destroy(kernel);

	assert(test7_lowest_priority_counter == 1 && test7_average_priority_counter == 1 && test7_highest_priority_counter == 1);

	return 0;
}
