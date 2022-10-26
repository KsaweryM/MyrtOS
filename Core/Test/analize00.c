#include "analize.h"
#include <kernel/kernel.h>
#include <kernel/thread.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

//#define ANALIZE0_DEBUG

TIM_HandleTypeDef htim3;

volatile uint32_t analize0_end = 0;
volatile uint32_t tim3_counter = 0;

void TIM3_IRQHandler(void)
{
  /* USER CODE BEGIN TIM3_IRQn 0 */


	analize0_end++;
	//tim3_counter++;
  /* USER CODE END TIM3_IRQn 0 */
  HAL_TIM_IRQHandler(&htim3);
  /* USER CODE BEGIN TIM3_IRQn 1 */

  /* USER CODE END TIM3_IRQn 1 */

  if (analize0_end == 2)
  HAL_TIM_Base_Stop_IT(&htim3);
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

void analize0_counter(void* atr)
{
	volatile uint32_t* counter = (uint32_t*) atr;

  while(analize0_end != 2)
  {
    (*counter)++;
  }
}

void analize00(SCHEDULER_ALGORITHM scheduler_algorithm)
{
	printf("nr_threads;thread_id;counter_of_thread;sum_of_counters;period_in_ms;algorithm\r\n");

	register const uint32_t test_time_in_seconds = 16;

	uint32_t contex_periods[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
	uint32_t contex_periods_size = sizeof(contex_periods) / sizeof(contex_periods[0]);

	for (uint32_t contex_id = 0; contex_id < contex_periods_size; contex_id++)
	{
		for (uint32_t nr_threads = 1; nr_threads <= 5; nr_threads++)
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

			uint32_t total = 0;
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
	}
}
