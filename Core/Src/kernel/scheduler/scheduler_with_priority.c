#include <kernel/scheduler/scheduler_p.h>
#include <kernel/scheduler/scheduler_with_priority.h>
#include <kernel/list.h>
#include <kernel/atomic.h>
#include <kernel/kernel.h>
#include <kernel/blocker.h>
#include <stdlib.h>

#define NUMBER_PRIORITIES 16

typedef enum SCHEDULER_WITH_PRIORITY_STATE
{
	SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD,
	SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD,
	DEACTIVATE_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD,
	SUSPEND_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD,
	BLOCK_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD,
	NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD
} SCHEDULER_WITH_PRIORITY_STATE;

struct scheduler_with_priority_t
{
	scheduler_t scheduler;
	list_t** threads_lists;
	list_t* deactivated_threads;
	iterator_t** threads_iterators;
	uint32_t* main_thread_SP_register;
	thread_control_block_t* current_thread;
	blocker_t* blocker;
	SCHEDULER_WITH_PRIORITY_STATE state;
	uint32_t is_launched;
};

scheduler_with_priority_t* scheduler_with_priority_g = 0;

static void __scheduler_with_priority_destroy(scheduler_t* scheduler);
static uint32_t* __scheduler_with_priority_choose_next_thread(scheduler_t* scheduler, uint32_t* SP_register);
static void __scheduler_with_priority_add_thread(scheduler_t* scheduler, const thread_attributes_t* thread_attributes);
static void __scheduler_with_priority_add_thread_control_block(scheduler_t* scheduler, thread_control_block_t* thread_control_block);
static void __scheduler_with_priority_block_thread(scheduler_t* scheduler, uint32_t delay);
static void __scheduler_with_priority_change_thread_priority(scheduler_t* scheduler, thread_control_block_t* thread_control_block, uint32_t priority);
static uint32_t  __scheduler_with_priority_get_nr_priorities(scheduler_t* scheduler);
static thread_control_block_t* __scheduler_with_priority_get_current_thread(scheduler_t* scheduler);
static void  __scheduler_with_priority_set_mutex_state(scheduler_t* scheduler);
static void  __scheduler_with_priority_launch(scheduler_t* scheduler);

static void __scheduler_with_priority_deactivate_current_thread(void);
static void __scheduler_with_priority_destroy_deactivated_threads(scheduler_t* scheduler);
static void __scheduler_with_priority_destroy_list_content(list_t* list);

scheduler_with_priority_t* scheduler_with_priority_create()
{
	CRITICAL_PATH_ENTER();

	if (scheduler_with_priority_g == 0)
	{
		scheduler_with_priority_g = malloc(sizeof(*scheduler_with_priority_g));

		scheduler_with_priority_g->scheduler.scheduler_destroy = __scheduler_with_priority_destroy;
		scheduler_with_priority_g->scheduler.scheduler_choose_next_thread = __scheduler_with_priority_choose_next_thread;
		scheduler_with_priority_g->scheduler.scheduler_add_thread = __scheduler_with_priority_add_thread;
		scheduler_with_priority_g->scheduler.scheduler_add_thread_control_block = __scheduler_with_priority_add_thread_control_block;
		scheduler_with_priority_g->scheduler.scheduler_block_thread = __scheduler_with_priority_block_thread;
		scheduler_with_priority_g->scheduler.scheduler_change_thread_priority = __scheduler_with_priority_change_thread_priority;
		scheduler_with_priority_g->scheduler.scheduler_get_nr_priorities = __scheduler_with_priority_get_nr_priorities;
		scheduler_with_priority_g->scheduler.scheduler_get_current_thread = __scheduler_with_priority_get_current_thread;
		scheduler_with_priority_g->scheduler.scheduler_set_mutex_state = __scheduler_with_priority_set_mutex_state;
		scheduler_with_priority_g->scheduler.scheduler_launch = __scheduler_with_priority_launch;

		scheduler_with_priority_g->threads_lists = malloc(sizeof(*scheduler_with_priority_g->threads_lists) * NUMBER_PRIORITIES);
		scheduler_with_priority_g->deactivated_threads = list_create();
		scheduler_with_priority_g->threads_iterators = malloc(sizeof(*scheduler_with_priority_g->threads_iterators) * NUMBER_PRIORITIES);
		scheduler_with_priority_g->main_thread_SP_register = 0;
		scheduler_with_priority_g->current_thread = 0;
		scheduler_with_priority_g->blocker = blocker_create((scheduler_t*) scheduler_with_priority_g);

		scheduler_with_priority_g->state = SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
		scheduler_with_priority_g->is_launched = 0;

		for (uint32_t i = 0; i < NUMBER_PRIORITIES; i++)
		{
			scheduler_with_priority_g->threads_lists[i] = list_create();
			scheduler_with_priority_g->threads_iterators[i] = iterator_create(scheduler_with_priority_g->threads_lists[i]);
		}
	}

	CRITICAL_PATH_EXIT();

	return scheduler_with_priority_g;
}

static void __scheduler_with_priority_destroy(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	scheduler_with_priority_t* scheduler_with_priority = (scheduler_with_priority_t*) scheduler;

	__scheduler_with_priority_destroy_list_content(scheduler_with_priority->deactivated_threads);
	list_destroy(scheduler_with_priority->deactivated_threads);

	for (uint32_t i = 0; i < NUMBER_PRIORITIES; i++)
	{
		iterator_destroy(scheduler_with_priority->threads_iterators[i]);

		__scheduler_with_priority_destroy_list_content(scheduler_with_priority->threads_lists[i]);
		list_destroy(scheduler_with_priority->threads_lists[i]);
	}

	free(scheduler_with_priority->threads_lists);
	free(scheduler_with_priority->threads_iterators);

	thread_control_block_destroy(scheduler_with_priority->current_thread);
	blocker_destroy(scheduler_with_priority->blocker);

	free(scheduler_with_priority);

	scheduler_with_priority_g = 0;

	CRITICAL_PATH_EXIT();
}

static uint32_t* __scheduler_with_priority_choose_next_thread(scheduler_t* scheduler, uint32_t* SP_register)
{
	CRITICAL_PATH_ENTER();

	scheduler_with_priority_t* scheduler_with_priority = (scheduler_with_priority_t*) scheduler;

	for (uint32_t i = 0; i < NUMBER_PRIORITIES; i++)
	{
		iterator_update(scheduler_with_priority->threads_iterators[i]);
	}

	if (scheduler_with_priority->state == SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD)
	{
		scheduler_with_priority->main_thread_SP_register = SP_register;

		for (uint32_t i = 0; i < NUMBER_PRIORITIES; i++)
		{
			iterator_reset(scheduler_with_priority->threads_iterators[i]);
		}

		int32_t iterator_index = NUMBER_PRIORITIES - 1;

		do
		{
			iterator_t* iterator = scheduler_with_priority->threads_iterators[iterator_index];
			scheduler_with_priority->current_thread = (thread_control_block_t*)iterator_get_data(iterator);
			iterator_index--;

		} while (scheduler_with_priority->current_thread == 0 && iterator_index >= 0);

		scheduler_with_priority->state = (scheduler_with_priority->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_with_priority->state == SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD)
	{
		thread_control_block_set_stack_pointer(scheduler_with_priority->current_thread, SP_register);

		int32_t iterator_index = NUMBER_PRIORITIES - 1;

		do
		{
			iterator_t* iterator = scheduler_with_priority->threads_iterators[iterator_index];

			cyclic_iterator_next(iterator);
			scheduler_with_priority->current_thread = (thread_control_block_t*)iterator_get_data(iterator);
			iterator_index--;

		} while (scheduler_with_priority->current_thread == 0 && iterator_index >= 0);

		scheduler_with_priority->state = (scheduler_with_priority->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_with_priority->state == DEACTIVATE_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD)
	{

		int32_t iterator_index = NUMBER_PRIORITIES - 1;

		do
		{
			iterator_t* iterator = scheduler_with_priority->threads_iterators[iterator_index];

			scheduler_with_priority->current_thread = (thread_control_block_t*)iterator_get_data(iterator);
			iterator_index--;

		} while (scheduler_with_priority->current_thread == 0 && iterator_index >= 0);

		scheduler_with_priority->state = (scheduler_with_priority->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_with_priority->state == SUSPEND_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD)
	{
		uint32_t thread_priority = thread_control_block_get_priority(scheduler_with_priority->current_thread);
		thread_control_block_t* suspended_thread = iterator_pop(scheduler_with_priority->threads_iterators[thread_priority]);
		thread_control_block_set_stack_pointer(suspended_thread, SP_register);

		int32_t iterator_index = NUMBER_PRIORITIES - 1;

		do
		{
			iterator_t* iterator = scheduler_with_priority->threads_iterators[iterator_index];

			cyclic_iterator_next(iterator);
			scheduler_with_priority->current_thread = (thread_control_block_t*)iterator_get_data(iterator);
			iterator_index--;

		} while (scheduler_with_priority->current_thread == 0 && iterator_index >= 0);

		scheduler_with_priority->state = (scheduler_with_priority->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_with_priority->state == BLOCK_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD)
	{
		uint32_t thread_priority = thread_control_block_get_priority(scheduler_with_priority->current_thread);
		thread_control_block_t* blocked_thread = (thread_control_block_t*) cyclic_iterator_pop(scheduler_with_priority->threads_iterators[thread_priority]);
		thread_control_block_set_stack_pointer(blocked_thread, SP_register);

		blocker_block_thread(scheduler_with_priority->blocker, blocked_thread);

		int32_t iterator_index = NUMBER_PRIORITIES - 1;

		do
		{
			iterator_t* iterator = scheduler_with_priority->threads_iterators[iterator_index];

			cyclic_iterator_next(iterator);
			scheduler_with_priority->current_thread = (thread_control_block_t*)iterator_get_data(iterator);
			iterator_index--;

		} while (scheduler_with_priority->current_thread == 0 && iterator_index >= 0);

		scheduler_with_priority->state = (scheduler_with_priority->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}

	if (scheduler_with_priority->state == NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD)
	{
		__scheduler_with_priority_destroy_deactivated_threads(scheduler);

		scheduler_with_priority->state = SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}

	uint32_t* next_SP_register = ((scheduler_with_priority->current_thread == 0) ?
			scheduler_with_priority->main_thread_SP_register : thread_control_block_get_stack_pointer(scheduler_with_priority->current_thread));

	CRITICAL_PATH_EXIT();

	return next_SP_register;
}

static void __scheduler_with_priority_add_thread(scheduler_t* scheduler, const thread_attributes_t* thread_attributes)
{
	CRITICAL_PATH_ENTER();

	thread_control_block_t* thread_control_block = thread_control_block_create(thread_attributes, __scheduler_with_priority_deactivate_current_thread);
	scheduler_with_priority_t* scheduler_with_priority = (scheduler_with_priority_t*) scheduler;
	list_push_back(scheduler_with_priority->threads_lists[thread_attributes->thread_priority], thread_control_block);

	if (scheduler_with_priority->is_launched)
	{
		yield_after_critical_path();
	}

	CRITICAL_PATH_EXIT();
}

static void __scheduler_with_priority_add_thread_control_block(scheduler_t* scheduler, thread_control_block_t* thread_control_block)
{
	CRITICAL_PATH_ENTER();

	scheduler_with_priority_t* scheduler_with_priority = (scheduler_with_priority_t*) scheduler;
	uint32_t thread_priority = thread_control_block_get_priority(thread_control_block);
	list_push_back(scheduler_with_priority->threads_lists[thread_priority], thread_control_block);

	if (scheduler_with_priority->is_launched)
	{
		yield_after_critical_path();
	}

	CRITICAL_PATH_EXIT();
}

static void  __scheduler_with_priority_block_thread(scheduler_t* scheduler, uint32_t delay)
{
	CRITICAL_PATH_ENTER();

	scheduler_with_priority_t* scheduler_with_priority = (scheduler_with_priority_t*) scheduler;
	thread_control_block_set_delay(scheduler_with_priority->current_thread, delay);
	scheduler_with_priority->state = BLOCK_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD;

	CRITICAL_PATH_EXIT();

	yield();
}

static void __scheduler_with_priority_change_thread_priority(scheduler_t* scheduler, thread_control_block_t* thread_control_block, uint32_t priority)
{
	CRITICAL_PATH_ENTER();

	scheduler_with_priority_t* scheduler_with_priority = (scheduler_with_priority_t*) scheduler;

	uint32_t current_priority = thread_control_block_get_priority(thread_control_block);

	iterator_remove_item_by_data(scheduler_with_priority->threads_iterators[current_priority], thread_control_block);

	if (iterator_get_data(scheduler_with_priority->threads_iterators[current_priority]) == 0)
	{
	cyclic_iterator_next(scheduler_with_priority->threads_iterators[current_priority]);
	}

	thread_control_block_set_priority(thread_control_block, priority);

	__scheduler_with_priority_add_thread_control_block((scheduler_t*) scheduler_with_priority, thread_control_block);

	CRITICAL_PATH_EXIT();
}

static uint32_t __scheduler_with_priority_get_nr_priorities(scheduler_t* scheduler)
{
	return NUMBER_PRIORITIES;
}

static thread_control_block_t* __scheduler_with_priority_get_current_thread(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	assert(scheduler == (scheduler_t*) scheduler_with_priority_g);

	scheduler_with_priority_t* scheduler_with_priority = (scheduler_with_priority_t*) scheduler;

	thread_control_block_t* current_thread = scheduler_with_priority->current_thread;

	CRITICAL_PATH_EXIT();

	return current_thread;
}

static void  __scheduler_with_priority_set_mutex_state(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	assert(scheduler == (scheduler_t*) scheduler_with_priority_g);

	scheduler_with_priority_t* scheduler_with_priority = (scheduler_with_priority_t*) scheduler;

	scheduler_with_priority->state = SUSPEND_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD;

	CRITICAL_PATH_EXIT();
}

static void __scheduler_with_priority_launch(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	scheduler_with_priority_t* scheduler_with_priority = (scheduler_with_priority_t*) scheduler;

	scheduler_with_priority->is_launched = 1;

	CRITICAL_PATH_EXIT();
}

static void __scheduler_with_priority_deactivate_current_thread(void)
{
	CRITICAL_PATH_ENTER();

	uint32_t current_thread_priority = thread_control_block_get_priority(scheduler_with_priority_g->current_thread);
	thread_control_block_t* deactivated_thread = cyclic_iterator_pop(scheduler_with_priority_g->threads_iterators[current_thread_priority]);
	list_push_back(scheduler_with_priority_g->deactivated_threads, deactivated_thread);

	scheduler_with_priority_g->state = DEACTIVATE_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD;

	CRITICAL_PATH_EXIT();

	yield();

	assert(0);
}

static void __scheduler_with_priority_destroy_deactivated_threads(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	scheduler_with_priority_t* scheduler_with_priority = (scheduler_with_priority_t*) scheduler;

	__scheduler_with_priority_destroy_list_content(scheduler_with_priority->deactivated_threads);

	CRITICAL_PATH_EXIT();
}

static void __scheduler_with_priority_destroy_list_content(list_t* list)
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

