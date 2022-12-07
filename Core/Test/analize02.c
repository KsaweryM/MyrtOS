#include "analize.h"
#include <kernel/kernel.h>
#include <kernel/mutex/mutex.h>
#include <kernel/atomic.h>
#include <kernel/thread.h>
#include <kernel/atomic.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <kernel/measure_time.h>
#include <string.h>

volatile uint32_t analize2_total_max = 10000;
volatile uint32_t analize2_total = 0;
volatile uint32_t analize2_end = 0;
const uint32_t analize2_sleep_time = 1;
volatile uint32_t analize2_nr_high_priority_threads = 0;
volatile uint32_t analize2_nr_finished_high_priority_threads = 0;

void analize2_high_priority_thread(void* args);
void analize2_low_priority_thread(void* args);


void analize2_high_priority_thread(void* args)
{
	volatile uint32_t* my_counter = ((uint32_t*) args);

	while (analize2_total < analize2_total_max)
	{
		analize2_total++;
		(*my_counter)++;
		delay(analize2_sleep_time);
	}

	analize2_nr_finished_high_priority_threads++;
}

void analize2_low_priority_thread(void* args)
{
	volatile uint32_t* my_counter = ((uint32_t*) args);

	while (analize2_total < analize2_total_max)
	{
		analize2_total++;
		(*my_counter)++;
		yield();
	}
}


void wait_for_rest_threads(void* args)
{
	while (analize2_nr_finished_high_priority_threads < analize2_nr_high_priority_threads)
	{
		yield();
	}
}

void analize02(SCHEDULER_ALGORITHM scheduler_algorithm)
{

	uint32_t nr_threads = 50;
	uint32_t increment = 1;
	for (uint32_t ii = 0; ii <= 49; ii += increment)
	{
		uint32_t total_total_high_priority = 0;
		uint32_t total_total_low_priority = 0;

		uint32_t nr_high_threads = ii;

		//printf("nr_high_threads = %ld\r\n", nr_high_threads);

		//printf("times: ");
		for (uint32_t jj = 0; jj < 20; jj++)
		{
		//printf("%ld \r\n", jj);
		analize2_total = 0;
		analize2_nr_finished_high_priority_threads = 0;






		analize2_nr_finished_high_priority_threads = 0;
		analize2_end = 0;

		kernel_attributes_t kernel_attributes_object = {
				.scheduler_algorithm = scheduler_algorithm
		};

		kernel_t* kernel = kernel_create(&kernel_attributes_object);

		uint32_t nr_low_threads = nr_threads - nr_high_threads;

		analize2_nr_high_priority_threads = nr_high_threads;

		thread_attributes_t* high_threads = malloc(sizeof(*high_threads) * nr_high_threads);
		thread_attributes_t* low_threads = malloc(sizeof(*low_threads) * nr_low_threads);

		uint32_t* high_counters = malloc(sizeof(*high_counters) * nr_high_threads);
		uint32_t* low_counters = malloc(sizeof(*low_counters) * nr_low_threads);

		memset(high_counters, 0, sizeof(*high_counters) * nr_high_threads);
		memset(low_counters, 0, sizeof(*low_counters) * nr_low_threads);

		for (uint32_t i = 0; i < nr_high_threads; i++)
		{
			high_threads[i].function = analize2_high_priority_thread;
			high_threads[i].function_arguments = (void*) &high_counters[i];
			high_threads[i].stack_size = 200;
			high_threads[i].thread_priority = 14;
			high_threads[i].thread_name = "high thread";

			kernel_add_thread(kernel, &high_threads[i]);
		}

		for (uint32_t i = 0; i < nr_low_threads; i++)
		{
			low_threads[i].function = analize2_low_priority_thread;
			low_threads[i].function_arguments = (void*) &low_counters[i];
			low_threads[i].stack_size = 200;
			low_threads[i].thread_priority = 13;
			low_threads[i].thread_name = "low thread";

			kernel_add_thread(kernel, &low_threads[i]);
		}


		thread_attributes_t thread_wait = {
				.function = wait_for_rest_threads,
				.function_arguments = 0,
				.stack_size = 500,
				.thread_priority = 0,
				.thread_name = "wait thread"
		};

		kernel_add_thread(kernel, &thread_wait);


		//measure_start();
		kernel_launch(kernel);

	//	printf("high priority threads:\r\n");
		uint32_t total_high_priority = 0;

		for (uint32_t i = 0; i < nr_high_threads; i++)
		{
			//printf("%ld;", high_counters[i]);

			total_high_priority += high_counters[i];
		}


		uint32_t total_low_priority = 0;

		//printf("low priority threads:\r\n");
		for (uint32_t i = 0; i < nr_low_threads; i++)
		{
			//printf("%ld;", low_counters[i]);
			total_low_priority += low_counters[i];
		}

		total_total_high_priority += total_high_priority;
		total_total_low_priority += total_low_priority;


		//printf("\r\n");

		//printf("total_high_priority = %ld, total_low_priority = %ld\r\n", total_high_priority, total_low_priority);

		kernel_destroy(kernel);

		free(high_counters);
		free(low_counters);
		free(high_threads);
		free(low_threads);
		}


		printf("%ld;=%ld/%ld\r\n", ii, total_total_high_priority, total_total_low_priority);
	}
	/*
	printf("\r\nmeasure;time;algorithm\r\n");


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
	*/
}
