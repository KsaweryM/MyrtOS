#include <kernel/scheduler/scheduler_p.h>
#include <kernel/scheduler/scheduler_priority_time_slicing.h>
#include <kernel/list.h>
#include <kernel/atomic.h>
#include <kernel/kernel.h>
#include <kernel/blocker.h>
#include <stdlib.h>

#define NUMBER_PRIORITIES 16

typedef enum SCHEDULER_PRIORITY_TIME_SLICING_STATE
{
	SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD,
	SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD,
	DEACTIVATE_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD,
	SUSPEND_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD,
	BLOCK_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD,
	NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD
} SCHEDULER_PRIORITY_TIME_SLICING_STATE;

struct scheduler_priority_time_slicing_t
{
	scheduler_t scheduler;
	list_t** threads_lists;
	list_t* deactivated_threads;
	iterator_t** threads_iterators;
	uint32_t* main_thread_SP_register;
	thread_control_block_t* current_thread;
	blocker_t* blocker;
	SCHEDULER_PRIORITY_TIME_SLICING_STATE state;
};

scheduler_priority_time_slicing_t* scheduler_priority_time_slicing_g = 0;

static void __scheduler_priority_time_slicing_destroy(scheduler_t* scheduler);
static uint32_t* __scheduler_priority_time_slicing_choose_next_thread(scheduler_t* scheduler, uint32_t* SP_register);
static void __scheduler_priority_time_slicing_add_thread(scheduler_t* scheduler, const thread_attributes_t* thread_attributes);
static void __scheduler_priority_time_slicing_add_thread_control_block(scheduler_t* scheduler, thread_control_block_t* thread_control_block);
static void __scheduler_priority_time_slicing_block_thread(scheduler_t* scheduler, uint32_t delay);
static void __scheduler_priority_time_slicing_change_thread_priority(scheduler_t* scheduler, thread_control_block_t* thread_control_block, uint32_t priority);
static uint32_t  __scheduler_priority_time_slicing_get_nr_priorities(scheduler_t* scheduler);
static thread_control_block_t* __scheduler_priority_time_slicing_get_current_thread(scheduler_t* scheduler);
static void  __scheduler_priority_time_slicing_set_mutex_state(scheduler_t* scheduler);

static void __scheduler_priority_time_slicing_deactivate_current_thread(void);
static void __scheduler_priority_time_slicing_destroy_deactivated_threads(scheduler_t* scheduler);
static void __scheduler_priority_time_slicing_destroy_list_content(list_t* list);

scheduler_priority_time_slicing_t* scheduler_priority_time_slicing_create()
{
	CRITICAL_PATH_ENTER();

	if (scheduler_priority_time_slicing_g == 0)
	{
		scheduler_priority_time_slicing_g = malloc(sizeof(*scheduler_priority_time_slicing_g));

		scheduler_priority_time_slicing_g->scheduler.scheduler_destroy = __scheduler_priority_time_slicing_destroy;
		scheduler_priority_time_slicing_g->scheduler.scheduler_choose_next_thread = __scheduler_priority_time_slicing_choose_next_thread;
		scheduler_priority_time_slicing_g->scheduler.scheduler_add_thread = __scheduler_priority_time_slicing_add_thread;
		scheduler_priority_time_slicing_g->scheduler.scheduler_add_thread_control_block = __scheduler_priority_time_slicing_add_thread_control_block;
		scheduler_priority_time_slicing_g->scheduler.scheduler_block_thread = __scheduler_priority_time_slicing_block_thread;
		scheduler_priority_time_slicing_g->scheduler.scheduler_change_thread_priority = __scheduler_priority_time_slicing_change_thread_priority;
		scheduler_priority_time_slicing_g->scheduler.scheduler_get_nr_priorities = __scheduler_priority_time_slicing_get_nr_priorities;
		scheduler_priority_time_slicing_g->scheduler.scheduler_get_current_thread = __scheduler_priority_time_slicing_get_current_thread;
		scheduler_priority_time_slicing_g->scheduler.scheduler_set_mutex_state = __scheduler_priority_time_slicing_set_mutex_state;


		scheduler_priority_time_slicing_g->threads_lists = malloc(sizeof(*scheduler_priority_time_slicing_g->threads_lists) * NUMBER_PRIORITIES);
		scheduler_priority_time_slicing_g->deactivated_threads = list_create();
		scheduler_priority_time_slicing_g->threads_iterators = malloc(sizeof(*scheduler_priority_time_slicing_g->threads_iterators) * NUMBER_PRIORITIES);
		scheduler_priority_time_slicing_g->main_thread_SP_register = 0;
		scheduler_priority_time_slicing_g->current_thread = 0;
		scheduler_priority_time_slicing_g->blocker = blocker_create((scheduler_t*) scheduler_priority_time_slicing_g);

		scheduler_priority_time_slicing_g->state = SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;

		for (uint32_t i = 0; i < NUMBER_PRIORITIES; i++)
		{
			scheduler_priority_time_slicing_g->threads_lists[i] = list_create();
			scheduler_priority_time_slicing_g->threads_iterators[i] = iterator_create(scheduler_priority_time_slicing_g->threads_lists[i]);
		}
	}

	CRITICAL_PATH_EXIT();

	return scheduler_priority_time_slicing_g;
}

static void __scheduler_priority_time_slicing_destroy(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	scheduler_priority_time_slicing_t* scheduler_priority_time_slicing = (scheduler_priority_time_slicing_t*) scheduler;

	__scheduler_priority_time_slicing_destroy_list_content(scheduler_priority_time_slicing->deactivated_threads);
	list_destroy(scheduler_priority_time_slicing->deactivated_threads);

	for (uint32_t i = 0; i < NUMBER_PRIORITIES; i++)
	{
		iterator_destroy(scheduler_priority_time_slicing->threads_iterators[i]);

		__scheduler_priority_time_slicing_destroy_list_content(scheduler_priority_time_slicing->threads_lists[i]);
		list_destroy(scheduler_priority_time_slicing->threads_lists[i]);
	}

	free(scheduler_priority_time_slicing->threads_lists);
	free(scheduler_priority_time_slicing->threads_iterators);

	thread_control_block_destroy(scheduler_priority_time_slicing->current_thread);
	blocker_destroy(scheduler_priority_time_slicing->blocker);

	free(scheduler_priority_time_slicing);

	scheduler_priority_time_slicing_g = 0;

	CRITICAL_PATH_EXIT();
}

static uint32_t* __scheduler_priority_time_slicing_choose_next_thread(scheduler_t* scheduler, uint32_t* SP_register)
{
	CRITICAL_PATH_ENTER();

	scheduler_priority_time_slicing_t* scheduler_priority_time_slicing = (scheduler_priority_time_slicing_t*) scheduler;

	if (scheduler_priority_time_slicing->state == SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD)
	{
		scheduler_priority_time_slicing->main_thread_SP_register = SP_register;

		for (uint32_t i = 0; i < NUMBER_PRIORITIES; i++)
		{
			iterator_reset(scheduler_priority_time_slicing->threads_iterators[i]);
		}

		int32_t iterator_index = NUMBER_PRIORITIES - 1;

		do
		{
			iterator_t* iterator = scheduler_priority_time_slicing->threads_iterators[iterator_index];
			scheduler_priority_time_slicing->current_thread = (thread_control_block_t*)iterator_get_data(iterator);
			iterator_index--;

		} while (scheduler_priority_time_slicing->current_thread == 0 && iterator_index >= 0);

		scheduler_priority_time_slicing->state = (scheduler_priority_time_slicing->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_priority_time_slicing->state == SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD)
	{
		thread_control_block_set_stack_pointer(scheduler_priority_time_slicing->current_thread, SP_register);

		int32_t iterator_index = NUMBER_PRIORITIES - 1;

		do
		{
			iterator_t* iterator = scheduler_priority_time_slicing->threads_iterators[iterator_index];

			cyclic_iterator_next(iterator);
			scheduler_priority_time_slicing->current_thread = (thread_control_block_t*)iterator_get_data(iterator);
			iterator_index--;

		} while (scheduler_priority_time_slicing->current_thread == 0 && iterator_index >= 0);

		scheduler_priority_time_slicing->state = (scheduler_priority_time_slicing->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_priority_time_slicing->state == DEACTIVATE_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD)
	{

		int32_t iterator_index = NUMBER_PRIORITIES - 1;

		do
		{
			iterator_t* iterator = scheduler_priority_time_slicing->threads_iterators[iterator_index];

			scheduler_priority_time_slicing->current_thread = (thread_control_block_t*)iterator_get_data(iterator);
			iterator_index--;

		} while (scheduler_priority_time_slicing->current_thread == 0 && iterator_index >= 0);

		scheduler_priority_time_slicing->state = (scheduler_priority_time_slicing->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_priority_time_slicing->state == SUSPEND_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD)
	{
		uint32_t thread_priority = thread_control_block_get_priority(scheduler_priority_time_slicing->current_thread);
		thread_control_block_t* suspended_thread = iterator_pop(scheduler_priority_time_slicing->threads_iterators[thread_priority]);
		thread_control_block_set_stack_pointer(suspended_thread, SP_register);

		int32_t iterator_index = NUMBER_PRIORITIES - 1;

		do
		{
			iterator_t* iterator = scheduler_priority_time_slicing->threads_iterators[iterator_index];

			cyclic_iterator_next(iterator);
			scheduler_priority_time_slicing->current_thread = (thread_control_block_t*)iterator_get_data(iterator);
			iterator_index--;

		} while (scheduler_priority_time_slicing->current_thread == 0 && iterator_index >= 0);

		scheduler_priority_time_slicing->state = (scheduler_priority_time_slicing->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_priority_time_slicing->state == BLOCK_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD)
	{
		uint32_t thread_priority = thread_control_block_get_priority(scheduler_priority_time_slicing->current_thread);
		thread_control_block_t* blocked_thread = (thread_control_block_t*) cyclic_iterator_pop(scheduler_priority_time_slicing->threads_iterators[thread_priority]);
		thread_control_block_set_stack_pointer(blocked_thread, SP_register);

		blocker_block_thread(scheduler_priority_time_slicing->blocker, blocked_thread);

		int32_t iterator_index = NUMBER_PRIORITIES - 1;

		do
		{
			iterator_t* iterator = scheduler_priority_time_slicing->threads_iterators[iterator_index];

			cyclic_iterator_next(iterator);
			scheduler_priority_time_slicing->current_thread = (thread_control_block_t*)iterator_get_data(iterator);
			iterator_index--;

		} while (scheduler_priority_time_slicing->current_thread == 0 && iterator_index >= 0);

		scheduler_priority_time_slicing->state = (scheduler_priority_time_slicing->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}

	if (scheduler_priority_time_slicing->state == NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD)
	{
		__scheduler_priority_time_slicing_destroy_deactivated_threads(scheduler);

		scheduler_priority_time_slicing->state = SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}

	uint32_t* next_SP_register = ((scheduler_priority_time_slicing->current_thread == 0) ?
			scheduler_priority_time_slicing->main_thread_SP_register : thread_control_block_get_stack_pointer(scheduler_priority_time_slicing->current_thread));

	CRITICAL_PATH_EXIT();

	return next_SP_register;
}

static void __scheduler_priority_time_slicing_add_thread(scheduler_t* scheduler, const thread_attributes_t* thread_attributes)
{
	CRITICAL_PATH_ENTER();

	thread_control_block_t* thread_control_block = thread_control_block_create(thread_attributes, __scheduler_priority_time_slicing_deactivate_current_thread);
	scheduler_priority_time_slicing_t* scheduler_priority_time_slicing = (scheduler_priority_time_slicing_t*) scheduler;
	list_push_back(scheduler_priority_time_slicing->threads_lists[thread_attributes->thread_priority], thread_control_block);

	CRITICAL_PATH_EXIT();
}

static void __scheduler_priority_time_slicing_add_thread_control_block(scheduler_t* scheduler, thread_control_block_t* thread_control_block)
{
	CRITICAL_PATH_ENTER();

	scheduler_priority_time_slicing_t* scheduler_priority_time_slicing = (scheduler_priority_time_slicing_t*) scheduler;
	uint32_t thread_priority = thread_control_block_get_priority(thread_control_block);
	list_push_back(scheduler_priority_time_slicing->threads_lists[thread_priority], thread_control_block);

	CRITICAL_PATH_EXIT();
}

static void  __scheduler_priority_time_slicing_block_thread(scheduler_t* scheduler, uint32_t delay)
{
	CRITICAL_PATH_ENTER();

	scheduler_priority_time_slicing_t* scheduler_priority_time_slicing = (scheduler_priority_time_slicing_t*) scheduler;
	thread_control_block_set_delay(scheduler_priority_time_slicing->current_thread, delay);
	scheduler_priority_time_slicing->state = BLOCK_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD;

	CRITICAL_PATH_EXIT();

	YIELD();
}

static void __scheduler_priority_time_slicing_change_thread_priority(scheduler_t* scheduler, thread_control_block_t* thread_control_block, uint32_t priority)
{
	CRITICAL_PATH_ENTER();

	scheduler_priority_time_slicing_t* scheduler_priority_time_slicing = (scheduler_priority_time_slicing_t*) scheduler;

	uint32_t current_priority = thread_control_block_get_priority(thread_control_block);

	iterator_remove_item_by_data(scheduler_priority_time_slicing->threads_iterators[current_priority], thread_control_block);

	if (iterator_get_data(scheduler_priority_time_slicing->threads_iterators[current_priority]) == 0)
	{
	cyclic_iterator_next(scheduler_priority_time_slicing->threads_iterators[current_priority]);
	}

	thread_control_block_set_priority(thread_control_block, priority);

	__scheduler_priority_time_slicing_add_thread_control_block((scheduler_t*) scheduler_priority_time_slicing, thread_control_block);

	CRITICAL_PATH_EXIT();
}

static uint32_t __scheduler_priority_time_slicing_get_nr_priorities(scheduler_t* scheduler)
{
	return NUMBER_PRIORITIES;
}

static thread_control_block_t* __scheduler_priority_time_slicing_get_current_thread(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	assert(scheduler == (scheduler_t*) scheduler_priority_time_slicing_g);

	scheduler_priority_time_slicing_t* scheduler_priority_time_slicing = (scheduler_priority_time_slicing_t*) scheduler;

	thread_control_block_t* current_thread = scheduler_priority_time_slicing->current_thread;

	CRITICAL_PATH_EXIT();

	return current_thread;
}

static void  __scheduler_priority_time_slicing_set_mutex_state(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	assert(scheduler == (scheduler_t*) scheduler_priority_time_slicing_g);

	scheduler_priority_time_slicing_t* scheduler_priority_time_slicing = (scheduler_priority_time_slicing_t*) scheduler;

	scheduler_priority_time_slicing->state = SUSPEND_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD;

	CRITICAL_PATH_EXIT();
}

static void __scheduler_priority_time_slicing_deactivate_current_thread(void)
{
	CRITICAL_PATH_ENTER();

	uint32_t current_thread_priority = thread_control_block_get_priority(scheduler_priority_time_slicing_g->current_thread);
	thread_control_block_t* deactivated_thread = cyclic_iterator_pop(scheduler_priority_time_slicing_g->threads_iterators[current_thread_priority]);
	list_push_back(scheduler_priority_time_slicing_g->deactivated_threads, deactivated_thread);

	scheduler_priority_time_slicing_g->state = DEACTIVATE_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD;

	CRITICAL_PATH_EXIT();

	YIELD();

	assert(0);
}

static void __scheduler_priority_time_slicing_destroy_deactivated_threads(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	scheduler_priority_time_slicing_t* scheduler_priority_time_slicing = (scheduler_priority_time_slicing_t*) scheduler;

	__scheduler_priority_time_slicing_destroy_list_content(scheduler_priority_time_slicing->deactivated_threads);

	CRITICAL_PATH_EXIT();
}

static void __scheduler_priority_time_slicing_destroy_list_content(list_t* list)
{
	CRITICAL_PATH_ENTER();

	iterator_t* iterator = iterator_create(list);

	while (1)
	{
		thread_control_block_t* thread_control_block = (thread_control_block_t*) cyclic_iterator_pop(iterator);

		if (thread_control_block == 0)
		{
			break;
		}

		thread_control_block_destroy(thread_control_block);
	}

	iterator_destroy(iterator);

	CRITICAL_PATH_EXIT();
}

