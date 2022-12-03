#include "analize.h"
#include <kernel/kernel.h>
#include <kernel/mutex/mutex.h>
#include <kernel/atomic.h>
#include <kernel/thread.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <kernel/measure_time.h>

uint32_t avg = 0;
uint32_t analize1_counter = 0;
kernel_t* analize1_kernel = 0;
mutex_t* analize1_mutex = 0;
uint32_t analize1_length = 1;
void analize1_thread0(void* atr);
void analize1_thread1(void* atr);
void analize1_thread2(void* atr);

thread_attributes_t analize1_thread0_atr = {
		.function = analize1_thread0,
		.function_arguments = 0,
		.stack_size = 1000,
		.thread_priority = 0
};

thread_attributes_t analize1_thread1_atr = {
		.function = analize1_thread1,
		.function_arguments = 0,
		.stack_size = 1000,
		.thread_priority = 1
};

thread_attributes_t analize1_thread2_atr = {
		.function = analize1_thread2,
		.function_arguments = 0,
		.stack_size = 1000,
		.thread_priority = 2
};

void analize1_thread0(void* atr)
{
	for (uint32_t i = 0; i < 1 * 10; i++)
	{
		mutex_lock(analize1_mutex);
		for (uint32_t i = 0; i < analize1_length * 1000; i++)
		{
			analize1_counter++;
		}

		kernel_add_thread(analize1_kernel, &analize1_thread1_atr);
		kernel_add_thread(analize1_kernel, &analize1_thread2_atr);
		mutex_unlock(analize1_mutex);
		kernel_destroy_deactivated_threads();
	}

}

void analize1_thread1(void* atr)
{
	mutex_lock(analize1_mutex);
	for (uint32_t i = 0; i < analize1_length * 1000; i++)
	{
		analize1_counter++;
	}

	mutex_unlock(analize1_mutex);
}

void analize1_thread2(void* atr)
{
	mutex_lock(analize1_mutex);
	for (uint32_t i = 0; i < analize1_length * 1000; i++)
	{
		analize1_counter++;
	}
	mutex_unlock(analize1_mutex);
}

void analize01(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	printf("\r\nmeasure;time;algorithm\r\n");
	for (uint32_t i = 0; i <= 10; i++)
	{
		analize1_counter = 0;
		analize1_length = i;
	kernel_attributes_t kernel_attributes_object = {
			.scheduler_algorithm = scheduler_algorithm
	};

	analize1_kernel = kernel_create(&kernel_attributes_object);

	analize1_mutex = mutex_create();
	kernel_add_thread(analize1_kernel, &analize1_thread0_atr);

	for (uint32_t j = 0; j < 1000; j++);

	measure_start();
	kernel_launch(analize1_kernel);
	uint32_t measure = measure_end();

	kernel_destroy(analize1_kernel);
	mutex_destroy(analize1_mutex);

	printf("%ld;%ld;%ld\r\n", measure,i, (uint32_t) scheduler_algorithm);
	//printf("avg_measure: %ld, algorithm: %ld, iterations: %ld \r\n", measure, (uint32_t) scheduler_algorithm, analize1_length);
	}
}
