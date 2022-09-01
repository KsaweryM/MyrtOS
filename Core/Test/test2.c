/*
 * The purpose of this test is thread_yield works correctly
 */

#include <kernel/kernel.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/atomic.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

#define TEST2_REPETITIONS 4
#define TEST2_END_VALUE 100

volatile int test2_counter = 0;
volatile int test2_d2 = 0;

void test2_task0(void* args)
{
	for (uint32_t i = 0; i < TEST2_END_VALUE; i++)
	{
		test2_counter++;
		thread_yield();
		test2_d2 = test2_d2 | test2_counter;
	}
}


void test2_task1(void* args)
{
	for (uint32_t i = 0; i < TEST2_END_VALUE; i++)
	{
		test2_counter=0;
		thread_yield();
	}
}

uint32_t test2()
{
	thread_attributes task0_attributes = {
		.thread_name = "task1",
		.function = test2_task0,
		.function_arguments = (void*)0,
		.stack_size = 1000,
		.thread_priority = 0
	};

	thread_attributes task1_attributes = {
		.thread_name = "task1",
		.function = test2_task1,
		.function_arguments = (void*)0,
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
	kernel_add_thread(kernel_object, &task1_attributes);

	kernel_launch(kernel_object);

	kernel_destroy(kernel_object);

	return 0;
}
