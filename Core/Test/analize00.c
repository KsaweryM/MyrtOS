#include "analize.h"
#include <kernel/kernel.h>
#include <kernel/thread.h>
#include <kernel/mutex/mutex.h>
#include <kernel/atomic.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#define MAX_NR_OF_THREADS 10

TIM_HandleTypeDef htim3;

volatile uint32_t analize0_total_nr_of_threads = 0;
volatile uint32_t analize0_current_nr_of_threads = 0;
volatile uint32_t analize0_total = 0;
volatile uint32_t analize0_end = 0;
mutex_t* analize0_mutex = 0;
kernel_t* analize0_kernel = 0;


volatile uint32_t priority_value = 1;

uint32_t get_priority()
{
	return 15 - (rand() % priority_value);
}
void TIM3_IRQHandler(void)
{
  /* USER CODE BEGIN TIM3_IRQn 0 */

	static uint32_t TIM3_counter = 0;
	TIM3_counter++;
  /* USER CODE END TIM3_IRQn 0 */
  HAL_TIM_IRQHandler(&htim3);
  /* USER CODE BEGIN TIM3_IRQn 1 */

  /* USER CODE END TIM3_IRQn 1 */

  if (TIM3_counter == 2)
  {
  	TIM3_counter = 0;
  	analize0_end = 1;
  	HAL_TIM_Base_Stop_IT(&htim3);
  }
}


static void MX_TIM3_Init(uint32_t period_in_seconds);
void analize0_counter(void* atg);

void analize0_counter(void* atg)
{
	uint32_t added = 0;
	uint32_t period = (uint32_t) atg;
	for (uint32_t i = 0; i < period; i++)
	{

		if (analize0_end)
		{
			break;
		}

		mutex_lock(analize0_mutex);
		analize0_total++;
		mutex_unlock(analize0_mutex);


		if (added == 0 && analize0_current_nr_of_threads < MAX_NR_OF_THREADS)
		{
			added = 1;
			thread_attributes_t attributes = {
					.function = analize0_counter,
					.function_arguments = atg,
					.stack_size = 1000,
					.function_arguments = "from thread",
					.thread_priority = get_priority()
			};

			analize0_total_nr_of_threads++;
			analize0_current_nr_of_threads++;

			kernel_add_thread(analize0_kernel, &attributes);
		}
	}

	ATOMIC_DECREMENT(analize0_current_nr_of_threads);

	CRITICAL_PATH_ENTER();
	kernel_destroy_deactivated_threads();
	CRITICAL_PATH_EXIT();
}

void analize00(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	printf("priority_value;contex_switch_in_ms;period;algorithm;total\r\n");

	uint32_t contex_periods[] = {10, 100};
	uint32_t periods[] = {1, 10, 100, 1000};
	uint32_t contex_size = sizeof(contex_periods) / sizeof(contex_periods[0]);
	uint32_t period_size = sizeof(periods) / sizeof(periods[0]);
	for (uint32_t j = 0; j < contex_size; j++)
	{
	for (uint32_t period_index = 0; period_index < period_size; period_index++)
	{
		uint32_t period = periods[period_index];
		priority_value = 1;
		while (priority_value < 16)
		{

				analize0_total = 0;
				analize0_end = 0;
				analize0_total_nr_of_threads = 0;
				analize0_current_nr_of_threads = 0;

				register const uint32_t test_in_seconds = 10;
				register const uint32_t nr_threads = 2;
				kernel_attributes_t kernel_attributes_object = {
						.scheduler_algorithm = scheduler_algorithm
				};

				analize0_kernel = kernel_create(&kernel_attributes_object);

				analize0_mutex = mutex_create();

				for (uint32_t i = 0; i < nr_threads; i++)
				{
					thread_attributes_t attributes = {
							.function = analize0_counter,
							.function_arguments = (void*) period,
							.stack_size = 1000,
							.thread_name = "from main",
							.thread_priority = get_priority()
					};

					kernel_add_thread(analize0_kernel, &attributes);

					analize0_total_nr_of_threads++;
					analize0_current_nr_of_threads++;
				}

				kernel_change_context_switch_period_in_milliseconds(analize0_kernel, contex_periods[j]);

				MX_TIM3_Init(test_in_seconds);
				HAL_TIM_Base_Start_IT(&htim3);

				kernel_launch(analize0_kernel);

				kernel_destroy(analize0_kernel);


				mutex_destroy(analize0_mutex);

				analize0_mutex = 0;
				printf("%ld;%ld;%ld;%ld;%ld\r\n",priority_value, contex_periods[j], period, (uint32_t) scheduler_algorithm, analize0_total);

			priority_value += 4;
		}
	}
	}
	/*
	printf("nr_threads;thread_id;counter_of_thread;sum_of_counters;period_in_ms;algorithm\r\n");

	register const uint32_t test_time_in_seconds = 32;

	uint32_t contex_periods[] = {1}; //, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
	uint32_t contex_periods_size = sizeof(contex_periods) / sizeof(contex_periods[0]);

	for (uint32_t contex_id = 0; contex_id < contex_periods_size; contex_id++)
	{
		for (uint32_t nr_threads = 1; nr_threads <= 1; nr_threads++)
		{
			#ifdef ANALIZE0_DEBUG
			printf("START\r\n");
			#endif

			analize0_end = 0;

			thread_attributes_t* counters = malloc(sizeof(*counters) * nr_threads);

			volatile uint64_t* counters_value = malloc(sizeof(*counters_value) * nr_threads);

			for (uint32_t thread = 0; thread < nr_threads; thread++)
			{
				counters_value[thread] = 0;

				counters[thread].function = analize0_counter;
				counters[thread].function_arguments = (void*) &counters_value[thread];
				counters[thread].stack_size = 2000;
				counters[thread].thread_priority = 14;

			}

			kernel_attributes_t kernel_attributes_object = {
					.scheduler_algorithm = scheduler_algorithm
			};

			kernel_t* kernel_object = kernel_create(&kernel_attributes_object);

			for (uint32_t thread = 0; thread < nr_threads; thread++)
			{
				kernel_add_thread(kernel_object, &counters[thread]);
			}

			uint32_t contex_period = contex_periods[contex_id];

			kernel_change_context_switch_period_in_milliseconds(kernel_object, contex_period);

			MX_TIM3_Init(test_time_in_seconds);
		  HAL_TIM_Base_Start_IT(&htim3);

			kernel_launch(kernel_object);

			#ifdef ANALIZE0_DEBUG
			printf("STOP\r\n");
			#endif

			kernel_destroy(kernel_object);

			uint64_t total = 0;
			for (uint32_t thread = 0; thread < nr_threads; thread++)
			{
				total += counters_value[thread];
			}

			for (uint32_t thread = 0; thread < nr_threads; thread++)
			{
				printf("%ld;%ld;%ld;%ld;%ld;%ld\r\n", nr_threads, thread, counters_value[thread], total, contex_period, (uint32_t) scheduler_algorithm);
			}

			free((void*) counters_value);

			free(counters);
		}
	}*/
}

static void MX_TIM3_Init(uint32_t period_in_seconds)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};


  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 4000 - 1;
  //htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period =  period_in_seconds * 1000 - 1;
  //htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    //Error_Handler();
  }


  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    //Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    //Error_Handler();
  }

}
