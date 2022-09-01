/*
 * The purpose of this test is to verify if the scheduler will perform the tasks assigned to it correctly (4 tasks, random order of adding tasks, random priorities of tasks)
 */

#include "kernel/kernel.h"
#include "kernel/thread.h"
#include <time.h>
#include <stdlib.h>
#include <assert.h>

#define TEST0_REPETITIONS 5
#define TIMER0_END 1000
#define TIMER1_END 1400
#define TIMER2_END 1200
#define TIMER3_END 2000

void test0_timer0(void* atr)
{
	uint32_t* arg = (uint32_t*) atr;

	uint32_t* counter = &arg[0];
  uint32_t* container = &arg[1];
  uint32_t* value_to_deliver = &arg[2];

  *container = *value_to_deliver;

  while(1)
  {
    (*counter)++;

    if ((*counter) == TIMER0_END)
    {
    	break;
    }
  }
}

void test0_timer1(void* atr)
{
	uint32_t* arg = (uint32_t*) atr;

	uint32_t* counter = &arg[0];
  uint32_t* container = &arg[1];
  uint32_t* value_to_deliver = &arg[2];

  while(1)
  {
    (*counter) += (*counter) + 1;

    if ((*counter) >= TIMER1_END)
    {
    	*container = *value_to_deliver;

    	break;
    }
  }
}

void test0_timer2(void* atr)
{
	uint32_t* arg = (uint32_t*) atr;

	uint32_t* counter = &arg[0];
  uint32_t* container = &arg[1];
  uint32_t* value_to_deliver = &arg[2];

  while(1)
  {
    (*counter)++;

    if ((*counter) == TIMER2_END)
    {
    	break;
    }
  }

  *container = *value_to_deliver;
}

void test0_timer3(void* atr)
{
	uint32_t* arg = (uint32_t*) atr;

	uint32_t* counter = &arg[0];
  uint32_t* container = &arg[1];
  uint32_t* value_to_deliver = &arg[2];
  uint32_t* end = &arg[3];

  *container = *value_to_deliver;

  uint32_t unused_array[20];

  (void)unused_array;

  while(1)
  {
    (*counter)++;
    *end = 0;

    if ((*counter) == TIMER3_END)
    {
    	*end = 1;
    	break;
    }
  }
}

uint32_t test0()
{
	for (uint32_t i = 0; i < TEST0_REPETITIONS; i++)
	{
		uint32_t timer0_args[] = {0, 0, 0};
		uint32_t timer1_args[] = {0, 0, 1};
		uint32_t timer2_args[] = {0, 0, 2};
		uint32_t timer3_args[] = {0, 0, 3, 0};

		uint32_t* timer0_counter = &timer0_args[0];
		uint32_t* timer1_counter = &timer1_args[0];
		uint32_t* timer2_counter = &timer2_args[0];
		uint32_t* timer3_counter = &timer3_args[0];

		uint32_t* timer0_delivered_value =  &timer0_args[1];
		uint32_t* timer1_delivered_value =  &timer1_args[1];
		uint32_t* timer2_delivered_value =  &timer2_args[1];
		uint32_t* timer3_delivered_value =  &timer3_args[1];

		uint32_t* timer0_value_to_deliver =  &timer0_args[2];
		uint32_t* timer1_value_to_deliver =  &timer1_args[2];
		uint32_t* timer2_value_to_deliver =  &timer2_args[2];
		uint32_t* timer3_value_to_deliver =  &timer3_args[2];

		uint32_t end_counter = 0;
		uint32_t* end = &timer3_args[3];

		thread_attributes timer0_attributes = {
			.function = test0_timer0,
			.function_arguments = (void*)&timer0_args,
			.stack_size = 1000,
			.thread_priority = rand() % 16
		};

		thread_attributes timer1_attributes = {
			.function = test0_timer1,
			.function_arguments = (void*)&timer1_args,
			.stack_size = 1000,
			.thread_priority = rand() % 16
		};

		thread_attributes timer2_attributes = {
			.function = test0_timer2,
			.function_arguments = (void*)&timer2_args,
			.stack_size = 1000,
			.thread_priority = rand() % 16
		};

		thread_attributes timer3_attributes = {
			.function = test0_timer3,
			.function_arguments = (void*)&timer3_args,
			.stack_size = 1000,
			.thread_priority = rand() % 16
		};

		kernel_attributes kernel_attributes_object = {

		};

		scheduler_attributes scheduler_attributes_object = {
			.algorithm = ROUND_ROBIN_SCHEDULING
		};

		kernel* kernel_object = kernel_create(&kernel_attributes_object, &scheduler_attributes_object);

		thread_attributes threads_attributes[] = {timer0_attributes, timer1_attributes, timer2_attributes, timer3_attributes};
		register const uint32_t nr_threads = sizeof(threads_attributes) / sizeof(threads_attributes[0]);
		uint32_t used[] = {0, 0, 0, 0};
		uint32_t nr_used = 0;

		while (nr_used != nr_threads)
		{
			uint32_t index = rand() % nr_threads;
			if (!used[index])
			{
				kernel_add_thread(kernel_object, &threads_attributes[index]);
				used[index] = 1;
				nr_used++;
			}
		}

		kernel_launch(kernel_object);

		do
		{
			end_counter++;
		} while (!(*end));

		kernel_destroy(kernel_object);

		uint32_t timer0_asert0 = (*timer0_counter == TIMER0_END);
		uint32_t timer1_asert0 = (*timer1_counter >= TIMER1_END);
		uint32_t timer2_asert0 = (*timer2_counter == TIMER2_END);
		uint32_t timer3_asert0 = (*timer3_counter == TIMER3_END);

		uint32_t timer0_asert1 = (*timer0_delivered_value == *timer0_value_to_deliver);
		uint32_t timer1_asert1 = (*timer1_delivered_value == *timer1_value_to_deliver);
		uint32_t timer2_asert1 = (*timer2_delivered_value == *timer2_value_to_deliver);
		uint32_t timer3_asert1 = (*timer3_delivered_value == *timer3_value_to_deliver);

		assert(timer0_asert0);
		assert(timer1_asert0);
		assert(timer2_asert0);
		assert(timer3_asert0);

		assert(timer0_asert1);
		assert(timer1_asert1);
		assert(timer2_asert1);
		assert(timer3_asert1);
	}

	return 0;
}
