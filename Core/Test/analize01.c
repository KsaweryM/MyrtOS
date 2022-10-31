#include "analize.h"
#include <kernel/kernel.h>
#include <kernel/thread.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <kernel/measure_time.h>
#include <kernel/atomic.h>

uint32_t avg = 0;

void analize1_thread0(void* atr)
{
	yield();
	measure_start();
}

void analize1_thread1(void* atr)
{
	yield();
	uint32_t end = measure_end();
	avg += end;
	//printf("measure: [%ld]\r\n", end);
}

void analize01(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	printf("Algorithm = %d\r\n",(int) scheduler_algorithm);
	avg = 0;
	uint32_t nr_threads = 100;
	for (uint32_t i = 0; i < nr_threads; i++)
	{
	thread_attributes_t thread0 = {
			.function = analize1_thread0,
			.function_arguments = 0,
			.stack_size = 1000,
			.thread_priority = 10
	};

	thread_attributes_t thread1 = {
			.function = analize1_thread1,
			.function_arguments = 0,
			.stack_size = 1000,
			.thread_priority = 10
	};

	kernel_attributes_t kernel_attributes_object = {
			.scheduler_algorithm = scheduler_algorithm
	};

	kernel_t* kernel_object = kernel_create(&kernel_attributes_object);

	kernel_add_thread(kernel_object, &thread0);
	kernel_add_thread(kernel_object, &thread1);

	kernel_launch(kernel_object);

	kernel_destroy(kernel_object);
	}

	avg /= nr_threads;
	printf("avg_measure: [%ld]\r\n", avg);
}
