#include "analize.h"
#include <kernel/kernel.h>
#include <kernel/thread.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

//#define ANALIZE0_DEBUG

volatile uint32_t analize0_end = 0;

void analize0_counter(void* atr)
{
	volatile uint32_t* counter = (uint32_t*) atr;

  while(!analize0_end)
  {
    (*counter)++;
  }
}

void analize0_timer(void* atr)
{
	uint32_t* delay_interval = (uint32_t*) atr;

	delay(*delay_interval);

	analize0_end = 1;
}

void analize00(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	printf("nr_threads; thread_id; counter_of_thread; sum_of_counters; period_in_ms\r\n");

	register const uint32_t test_time_in_seconds = 16;
	register uint32_t test_time_in_ms = test_time_in_seconds * 1000;

	uint32_t contex_periods[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
	uint32_t contex_periods_size = sizeof(contex_periods) / sizeof(contex_periods[0]);

	for (uint32_t contex_id = 0; contex_id < contex_periods_size; contex_id++)
	{
		for (uint32_t nr_threads = 4; nr_threads < 5; nr_threads++)
		{
			#ifdef ANALIZE0_DEBUG
			printf("START\r\n");
			#endif

			analize0_end = 0;

			thread_attributes_t* counters = malloc(sizeof(*counters) * nr_threads);

			volatile uint32_t* counters_value = malloc(sizeof(*counters_value) * nr_threads);

			for (uint32_t thread = 0; thread < nr_threads; thread++)
			{
				counters_value[thread] = 0;

				counters[thread].function = analize0_counter;
				counters[thread].function_arguments = (void*) &counters_value[thread];
				counters[thread].stack_size = 2000;
				counters[thread].thread_priority = 14;

			}

			uint32_t delay_value = test_time_in_ms;
			thread_attributes_t timer = {
				.function = analize0_timer,
				.function_arguments = (void*) &delay_value,
				.stack_size = 1000,
				.thread_priority = 15
			};

			kernel_attributes_t kernel_attributes_object = {
					.scheduler_algorithm = scheduler_algorithm
			};

			kernel_t* kernel_object = kernel_create(&kernel_attributes_object);

			for (uint32_t thread = 0; thread < nr_threads; thread++)
			{
				kernel_add_thread(kernel_object, &counters[thread]);
			}

			uint32_t contex_period = contex_periods[contex_id];
			kernel_add_thread(kernel_object, &timer);

			kernel_change_context_switch_period_in_milliseconds(kernel_object, contex_period);

			kernel_launch(kernel_object);

			#ifdef ANALIZE0_DEBUG
			printf("STOP\r\n");
			#endif

			kernel_destroy(kernel_object);

			uint32_t total = 0;
			for (uint32_t thread = 0; thread < nr_threads; thread++)
			{
				total += counters_value[thread];
			}

			for (uint32_t thread = 0; thread < nr_threads; thread++)
			{
				printf("%ld;%ld;%ld;%ld;%ld\r\n", nr_threads, thread, counters_value[thread], total, contex_period);
			}

			printf("\r\n");

			free((void*) counters_value);

			free(counters);
		}
	}
}
