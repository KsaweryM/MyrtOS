#include <kernel/scheduler/scheduler_p.h>
#include <kernel/scheduler/scheduler_without_priority.h>
#include <kernel/list.h>
#include <kernel/atomic.h>
#include <kernel/kernel.h>
#include <kernel/blocker.h>
#include <stdlib.h>

typedef enum SCHEDULER_WITHOUT_PRIORITY_STATE
{
	SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD,
	SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD,
	DEACTIVATE_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD,
	SUSPEND_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD,
	BLOCK_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD,
	NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD
} SCHEDULER_WITHOUT_PRIORITY_STATE;

struct scheduler_without_priority_t
{
	scheduler_t scheduler;
  list_t* threads_list;
	list_t* deactivated_threads;
  iterator_t* threads_iterator;
  uint32_t* main_thread_SP_register;
	thread_control_block_t* current_thread;
	blocker_t* blocker;
	SCHEDULER_WITHOUT_PRIORITY_STATE state;
};

scheduler_without_priority_t* scheduler_without_priority_g = 0;

static void __scheduler_without_priority_destroy(scheduler_t* scheduler);
static uint32_t* __scheduler_without_priority_choose_next_thread(scheduler_t* scheduler, uint32_t* SP_register);
static void __scheduler_without_priority_add_thread(scheduler_t* scheduler, const thread_attributes_t* thread_attributes);
static void __scheduler_without_priority_add_thread_control_block(scheduler_t* scheduler, thread_control_block_t* thread_control_block);
static void  __scheduler_without_priority_block_thread(scheduler_t* scheduler, uint32_t delay);
static void __scheduler_without_priority_change_thread_priority(scheduler_t* scheduler, thread_control_block_t* thread_control_block, uint32_t priority);
static uint32_t __scheduler_without_priority_get_nr_priorities(scheduler_t* scheduler);
static thread_control_block_t* __scheduler_without_priority_get_current_thread(scheduler_t* scheduler);
static void __scheduler_without_priority_set_mutex_state(scheduler_t* scheduler);

static void __scheduler_without_priority_deactivate_current_thread(void);
static void __scheduler_without_priority_destroy_deactivated_threads(scheduler_t* scheduler);

scheduler_without_priority_t* scheduler_without_priority_create()
{
	CRITICAL_PATH_ENTER();

	if (scheduler_without_priority_g == 0)
	{
		scheduler_without_priority_g = malloc(sizeof(*scheduler_without_priority_g));

		scheduler_without_priority_g->scheduler.scheduler_destroy = __scheduler_without_priority_destroy;
		scheduler_without_priority_g->scheduler.scheduler_choose_next_thread = __scheduler_without_priority_choose_next_thread;
		scheduler_without_priority_g->scheduler.scheduler_add_thread = __scheduler_without_priority_add_thread;
		scheduler_without_priority_g->scheduler.scheduler_add_thread_control_block = __scheduler_without_priority_add_thread_control_block;
		scheduler_without_priority_g->scheduler.scheduler_block_thread = __scheduler_without_priority_block_thread;
		scheduler_without_priority_g->scheduler.scheduler_change_thread_priority = __scheduler_without_priority_change_thread_priority;
		scheduler_without_priority_g->scheduler.scheduler_get_nr_priorities = __scheduler_without_priority_get_nr_priorities;
		scheduler_without_priority_g->scheduler.scheduler_get_current_thread = __scheduler_without_priority_get_current_thread;
		scheduler_without_priority_g->scheduler.scheduler_set_mutex_state = __scheduler_without_priority_set_mutex_state;

		scheduler_without_priority_g->threads_list = list_create();
		scheduler_without_priority_g->deactivated_threads = list_create();
		scheduler_without_priority_g->threads_iterator = iterator_create(scheduler_without_priority_g->threads_list);
		scheduler_without_priority_g->main_thread_SP_register = 0;
		scheduler_without_priority_g->current_thread = 0;
		scheduler_without_priority_g->blocker = blocker_create((scheduler_t*) scheduler_without_priority_g);

		scheduler_without_priority_g->state = SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}

	CRITICAL_PATH_EXIT();

	return scheduler_without_priority_g;
}

static void __scheduler_without_priority_destroy(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	scheduler_without_priority_t* scheduler_without_priority = (scheduler_without_priority_t*) scheduler;

	list_t* scheduler_lists[] = {scheduler_without_priority->threads_list, scheduler_without_priority->deactivated_threads};
	register const uint32_t number_lists = sizeof(scheduler_lists) / sizeof(scheduler_lists[0]);

	for (uint32_t i = 0; i < number_lists; i++)
	{
		iterator_t* iterator = iterator_create(scheduler_lists[i]);

		while (1)
		{
			thread_control_block_t* thread_control_block = (thread_control_block_t*) iterator_pop(iterator);

			if (thread_control_block == 0)
			{
				break;
			}

			thread_control_block_destroy(thread_control_block);
		}

		list_destroy(scheduler_lists[i]);
		iterator_destroy(iterator);
	}

	iterator_destroy(scheduler_without_priority->threads_iterator);
	thread_control_block_destroy(scheduler_without_priority->current_thread);
	blocker_destroy(scheduler_without_priority->blocker);

	free(scheduler_without_priority);

	scheduler_without_priority_g = 0;

	CRITICAL_PATH_EXIT();
}

static uint32_t* __scheduler_without_priority_choose_next_thread(scheduler_t* scheduler, uint32_t* SP_register)
{
	CRITICAL_PATH_ENTER();

	scheduler_without_priority_t* scheduler_without_priority = (scheduler_without_priority_t*) scheduler;

	iterator_update(scheduler_without_priority->threads_iterator);

	if (scheduler_without_priority->state == SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD)
	{
		scheduler_without_priority->main_thread_SP_register = SP_register;

		iterator_reset(scheduler_without_priority->threads_iterator);

		scheduler_without_priority->current_thread = (thread_control_block_t*)iterator_get_data(scheduler_without_priority->threads_iterator);

		scheduler_without_priority->state = (scheduler_without_priority->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_without_priority->state == SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD)
	{
		thread_control_block_set_stack_pointer(scheduler_without_priority->current_thread, SP_register);

		cyclic_iterator_next(scheduler_without_priority->threads_iterator);
		scheduler_without_priority->current_thread = (thread_control_block_t*)iterator_get_data(scheduler_without_priority->threads_iterator);

		scheduler_without_priority->state = (scheduler_without_priority->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_without_priority->state == DEACTIVATE_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD)
	{
		scheduler_without_priority->current_thread = (thread_control_block_t*)iterator_get_data(scheduler_without_priority->threads_iterator);

		scheduler_without_priority->state = (scheduler_without_priority->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_without_priority->state == SUSPEND_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD)
	{
		thread_control_block_set_stack_pointer(scheduler_without_priority->current_thread, SP_register);

		cyclic_iterator_pop(scheduler_without_priority->threads_iterator);

		scheduler_without_priority->current_thread = (thread_control_block_t*)iterator_get_data(scheduler_without_priority->threads_iterator);

		scheduler_without_priority->state = (scheduler_without_priority->current_thread == 0) ?
								NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_without_priority->state == BLOCK_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD)
	{
		thread_control_block_t* blocked_thread = (thread_control_block_t*) cyclic_iterator_pop(scheduler_without_priority->threads_iterator);
		thread_control_block_set_stack_pointer(blocked_thread, SP_register);

		blocker_block_thread(scheduler_without_priority->blocker, blocked_thread);

		scheduler_without_priority->current_thread = (thread_control_block_t*)iterator_get_data(scheduler_without_priority->threads_iterator);

		scheduler_without_priority->state = (scheduler_without_priority->current_thread == 0) ?
						NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}

	if (scheduler_without_priority->state == NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD)
	{
		__scheduler_without_priority_destroy_deactivated_threads(scheduler);

		scheduler_without_priority->state = SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}

	uint32_t* next_SP_register = ((scheduler_without_priority->current_thread == 0) ?
			scheduler_without_priority->main_thread_SP_register : thread_control_block_get_stack_pointer(scheduler_without_priority->current_thread));

	CRITICAL_PATH_EXIT();

	return next_SP_register;
}

static void __scheduler_without_priority_add_thread(scheduler_t* scheduler, const thread_attributes_t* thread_attributes)
{
	CRITICAL_PATH_ENTER();

	thread_control_block_t* thread_control_block = thread_control_block_create(thread_attributes, __scheduler_without_priority_deactivate_current_thread);
	scheduler_without_priority_t* scheduler_without_priority = (scheduler_without_priority_t*) scheduler;
	list_push_back(scheduler_without_priority->threads_list, thread_control_block);

	CRITICAL_PATH_EXIT();
}

static void __scheduler_without_priority_add_thread_control_block(scheduler_t* scheduler, thread_control_block_t* thread_control_block)
{
	CRITICAL_PATH_ENTER();

	scheduler_without_priority_t* scheduler_without_priority = (scheduler_without_priority_t*) scheduler;
	list_push_back(scheduler_without_priority->threads_list, thread_control_block);

	CRITICAL_PATH_EXIT();
}

static void  __scheduler_without_priority_block_thread(scheduler_t* scheduler, uint32_t delay)
{
	CRITICAL_PATH_ENTER();

	scheduler_without_priority_t* scheduler_without_priority = (scheduler_without_priority_t*) scheduler;
	thread_control_block_set_delay(scheduler_without_priority->current_thread, delay);
	scheduler_without_priority->state = BLOCK_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD;

	CRITICAL_PATH_EXIT();

	YIELD();
}

static void __scheduler_without_priority_change_thread_priority(scheduler_t* scheduler, thread_control_block_t* thread_control_block, uint32_t priority)
{

}

static uint32_t __scheduler_without_priority_get_nr_priorities(scheduler_t* scheduler)
{
	return 1;
}

static thread_control_block_t* __scheduler_without_priority_get_current_thread(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	assert(scheduler == (scheduler_t*) scheduler_without_priority_g);

	scheduler_without_priority_t* scheduler_without_priority = (scheduler_without_priority_t*) scheduler;

	thread_control_block_t* current_thread = scheduler_without_priority->current_thread;

	CRITICAL_PATH_EXIT();

	return current_thread;
}

static void __scheduler_without_priority_set_mutex_state(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	assert(scheduler == (scheduler_t*) scheduler_without_priority_g);

	scheduler_without_priority_t* scheduler_without_priority = (scheduler_without_priority_t*) scheduler;

	scheduler_without_priority->state = SUSPEND_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD;

	CRITICAL_PATH_EXIT();
}

static void __scheduler_without_priority_deactivate_current_thread(void)
{
	CRITICAL_PATH_ENTER();

	thread_control_block_t* deactivated_thread = cyclic_iterator_pop(scheduler_without_priority_g->threads_iterator);
	list_push_back(scheduler_without_priority_g->deactivated_threads, deactivated_thread);

	scheduler_without_priority_g->state = DEACTIVATE_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD;

	CRITICAL_PATH_EXIT();

	YIELD();

	assert(0);
}

static void __scheduler_without_priority_destroy_deactivated_threads(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	scheduler_without_priority_t* scheduler_without_priority = (scheduler_without_priority_t*) scheduler;

	iterator_t* deactivated_threads_iterator = iterator_create(scheduler_without_priority->deactivated_threads);

	while (1)
	{
		thread_control_block_t* thread_control_block = (thread_control_block_t*) iterator_pop(deactivated_threads_iterator);

		if (thread_control_block == 0)
		{
			break;
		}

		thread_control_block_destroy(thread_control_block);
	}

	iterator_destroy(deactivated_threads_iterator);

	CRITICAL_PATH_EXIT();
}
