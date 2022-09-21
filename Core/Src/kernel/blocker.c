#include <kernel/kernel.h>
#include <kernel/atomic.h>
#include <kernel/blocker.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/list.h>
#include <stm32l4xx.h>

#define TIM2_REINIT_COUNTER   (1U<<0)
#define TIM2_DOWNCOUNTER      (1U<<4)
#define TIM2_ENABLE_CLOCK			(1U<<0)
#define TIM2_ENABLE           (1U<<0)
#define TIM2_UIF							(1U<<0)
#define TIM2_ENABLE_INTERRUPT	(1U<<0)

#define TIM2_IS_PENDING_STATE (TIM2->SR & TIM2_UIF)

struct blocker_t
{
	scheduler_t* scheduler;
	list_t* blocked_threads;
	iterator_t* blocked_threads_iterator;
};

blocker_t* blocker_g = 0;

void __blocker_init_timer(void);
void __blocker_set_timer(uint32_t delay);
void __blocker_enable_timer(void);
void __blocker_disable_timer(void);

blocker_t* blocker_create(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	if (blocker_g == 0)
	{
		blocker_g = malloc(sizeof(*blocker_g));
		blocker_g->scheduler = scheduler;
		blocker_g->blocked_threads = list_create();
		blocker_g->blocked_threads_iterator = iterator_create(blocker_g->blocked_threads);

		__blocker_init_timer();
	}

	CRITICAL_PATH_EXIT();

	return blocker_g;
}

void blocker_destroy(blocker_t* blocker)
{
	CRITICAL_PATH_ENTER();

	assert(blocker != 0 && blocker == blocker_g);

	// blocked_threads list must be empty
	// maybe remove all thread from the list before destroying the list?
	assert(list_is_empty(blocker->blocked_threads));
	list_destroy(blocker->blocked_threads);
	iterator_destroy(blocker->blocked_threads_iterator);

	free(blocker);

	blocker_g = 0;

	__blocker_disable_timer();

	CRITICAL_PATH_EXIT();
}

void blocker_block_thread(blocker_t* blocker, thread_control_block_t* thread)
{
	CRITICAL_PATH_ENTER();

	__blocker_disable_timer();

	if (list_is_empty(blocker->blocked_threads))
	{
		list_push_back(blocker->blocked_threads, thread);
		__blocker_set_timer(thread_control_block_get_delay(thread));
	}
	else
	{
		iterator_reset(blocker->blocked_threads_iterator);
		thread_control_block_t* first_blocked_thread = iterator_get_data(blocker->blocked_threads_iterator);

		// update delay of first blocked thread in list
		uint32_t first_blocked_thread_delay = thread_control_block_get_delay(first_blocked_thread);

		uint32_t current_TIM2_value = TIM2->CNT;

		uint32_t first_blocked_thread_remaining_delay = (current_TIM2_value <= first_blocked_thread_delay) ? (first_blocked_thread_delay - current_TIM2_value) : 0;
		thread_control_block_set_delay(first_blocked_thread, first_blocked_thread_remaining_delay);

		TIM2->CNT = 0;

		// find a suitable place for new blocked thread

		uint32_t new_blocked_thread_delay = thread_control_block_get_delay(thread);

		while (1)
		{
			thread_control_block_t* blocked_thread = iterator_get_data(blocker->blocked_threads_iterator);

			if (blocked_thread == 0)
			{
				thread_control_block_set_delay(thread, new_blocked_thread_delay);
				list_push_back(blocker->blocked_threads, thread);

				break;
			}

			uint32_t blocked_thread_delay = thread_control_block_get_delay(blocked_thread);

			if (new_blocked_thread_delay > blocked_thread_delay)
			{
				new_blocked_thread_delay -= blocked_thread_delay;
			}
			else
			{
				blocked_thread_delay -= new_blocked_thread_delay;
				thread_control_block_set_delay(blocked_thread, blocked_thread_delay);

				thread_control_block_set_delay(thread, new_blocked_thread_delay);
				iterator_push_previous(blocker->blocked_threads_iterator, thread);

				break;
			}

			iterator_next(blocker->blocked_threads_iterator);
		}

		iterator_reset(blocker->blocked_threads_iterator);
		thread_control_block_t* current_first_blocked_thread = iterator_get_data(blocker->blocked_threads_iterator);
		uint32_t current_first_blocked_thread_delay = thread_control_block_get_delay(current_first_blocked_thread);
		__blocker_set_timer(current_first_blocked_thread_delay);
	}

	CRITICAL_PATH_EXIT();
}

void __blocker_init_timer(void)
{
  CRITICAL_PATH_ENTER();

  RCC->APB1ENR1 |= TIM2_ENABLE_CLOCK;
  TIM2->PSC = get_CPU_frequency() / 1000 - 1;

  TIM2->CR1 &= ~TIM2_ENABLE;

  TIM2->CR1 &= ~TIM2_DOWNCOUNTER;

  TIM2->EGR |= (1U<<0);

  TIM2->SR &= ~TIM2_UIF;

  TIM2->DIER |= TIM2_ENABLE_INTERRUPT;

  NVIC_EnableIRQ(TIM2_IRQn);

  CRITICAL_PATH_EXIT();
}

void __blocker_set_timer(uint32_t delay)
{
  CRITICAL_PATH_ENTER();

  TIM2->ARR = (delay <= 1) ? 1 : (delay - 1);

  TIM2->CNT = 0;

  TIM2->CR1 |= TIM2_ENABLE;

  CRITICAL_PATH_EXIT();
}

void __blocker_enable_timer(void)
{
  CRITICAL_PATH_ENTER();

  TIM2->CR1 |= TIM2_ENABLE;

  CRITICAL_PATH_EXIT();
}

void __blocker_disable_timer(void)
{
  CRITICAL_PATH_ENTER();

  TIM2->CR1 &= ~TIM2_ENABLE;

  CRITICAL_PATH_EXIT();
}

void TIM2_IRQHandler(void)
{
	CRITICAL_PATH_ENTER();

	__blocker_disable_timer();

  TIM2->SR &= ~TIM2_UIF;

  iterator_reset(blocker_g->blocked_threads_iterator);

  if (!list_is_empty(blocker_g->blocked_threads))
  {
		thread_control_block_t* thread = iterator_pop(blocker_g->blocked_threads_iterator);

		assert(thread);

		scheduler_add_thread_control_block(blocker_g->scheduler, thread);

		thread = iterator_get_data(blocker_g->blocked_threads_iterator);

		if (thread != 0)
		{
			uint32_t delay = thread_control_block_get_delay(thread);
			__blocker_set_timer(delay);
		}

  }

	CRITICAL_PATH_EXIT();
}
