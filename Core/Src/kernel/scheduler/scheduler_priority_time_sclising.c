#include <kernel/mutex_p.h>
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
	mutex_t* current_mutex;
	blocker_t* blocker;
	SCHEDULER_PRIORITY_TIME_SLICING_STATE state;
};

scheduler_priority_time_slicing_t* scheduler_priority_time_slicing_g = 0;

static void __scheduler_priority_time_slicing_destroy(scheduler_t* scheduler);
static uint32_t* __scheduler_priority_time_slicing_choose_next_thread(scheduler_t* scheduler, uint32_t* SP_register);
static void __scheduler_priority_time_slicing_add_thread(scheduler_t* scheduler, const thread_attributes_t* thread_attributes);
static void __scheduler_priority_time_slicing_add_thread_control_block(scheduler_t* scheduler, thread_control_block_t* thread_control_block);
static mutex_t* __scheduler_priority_time_slicing_create_mutex(scheduler_t* scheduler);

static void __scheduler_priority_time_slicing_deactivate_current_thread(void);
static void __scheduler_priority_time_slicing_destroy_deactivated_threads(scheduler_t* scheduler);
static void __scheduler_priority_time_slicing_destroy_list_content(list_t* list);
static void __scheduler_priority_time_slicing_block_thread(scheduler_t* scheduler, uint32_t delay);

static void __mutex_priority_time_slicing_lock(mutex_t* mutex);
static void __mutex_priority_time_slicing_unlock(mutex_t* mutex);
static void __mutex_priority_time_slicing_destroy(mutex_t* mutex);

scheduler_priority_time_slicing_t* scheduler_priority_time_slicing_create()
{
	if (scheduler_priority_time_slicing_g == 0)
	{
		scheduler_priority_time_slicing_g = malloc(sizeof(*scheduler_priority_time_slicing_g));

		scheduler_priority_time_slicing_g->scheduler.scheduler_destroy = __scheduler_priority_time_slicing_destroy;
		scheduler_priority_time_slicing_g->scheduler.scheduler_add_thread = __scheduler_priority_time_slicing_add_thread;
		scheduler_priority_time_slicing_g->scheduler.scheduler_add_thread_control_block = __scheduler_priority_time_slicing_add_thread_control_block;
		scheduler_priority_time_slicing_g->scheduler.scheduler_choose_next_thread = __scheduler_priority_time_slicing_choose_next_thread;
		scheduler_priority_time_slicing_g->scheduler.scheduler_create_mutex = __scheduler_priority_time_slicing_create_mutex;
		scheduler_priority_time_slicing_g->scheduler.scheduler_block_thread = __scheduler_priority_time_slicing_block_thread;

		scheduler_priority_time_slicing_g->threads_lists = malloc(sizeof(*scheduler_priority_time_slicing_g->threads_lists) * NUMBER_PRIORITIES);
		scheduler_priority_time_slicing_g->deactivated_threads = list_create();
		scheduler_priority_time_slicing_g->threads_iterators = malloc(sizeof(*scheduler_priority_time_slicing_g->threads_iterators) * NUMBER_PRIORITIES);
		scheduler_priority_time_slicing_g->main_thread_SP_register = 0;
		scheduler_priority_time_slicing_g->current_thread = 0;
		scheduler_priority_time_slicing_g->current_mutex = 0;
		scheduler_priority_time_slicing_g->blocker = blocker_create((scheduler_t*) scheduler_priority_time_slicing_g);

		scheduler_priority_time_slicing_g->state = SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;

		for (uint32_t i = 0; i < NUMBER_PRIORITIES; i++)
		{
			scheduler_priority_time_slicing_g->threads_lists[i] = list_create();
			scheduler_priority_time_slicing_g->threads_iterators[i] = iterator_create(scheduler_priority_time_slicing_g->threads_lists[i]);
		}
	}

	return scheduler_priority_time_slicing_g;
}

void __scheduler_priority_time_slicing_destroy(scheduler_t* scheduler)
{
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
}

void __scheduler_priority_time_slicing_add_thread(scheduler_t* scheduler, const thread_attributes_t* thread_attributes)
{
	thread_control_block_t* thread_control_block = thread_control_block_create(thread_attributes, __scheduler_priority_time_slicing_deactivate_current_thread);
	scheduler_priority_time_slicing_t* scheduler_priority_time_slicing = (scheduler_priority_time_slicing_t*) scheduler;
	list_push_back(scheduler_priority_time_slicing->threads_lists[thread_attributes->thread_priority], thread_control_block);
}

void __scheduler_priority_time_slicing_add_thread_control_block(scheduler_t* scheduler, thread_control_block_t* thread_control_block)
{
	scheduler_priority_time_slicing_t* scheduler_priority_time_slicing = (scheduler_priority_time_slicing_t*) scheduler;
	uint32_t thread_priority = thread_control_block_get_priority(thread_control_block);
	list_push_back(scheduler_priority_time_slicing->threads_lists[thread_priority], thread_control_block);
}

uint32_t* __scheduler_priority_time_slicing_choose_next_thread(scheduler_t* scheduler, uint32_t* SP_register)
{
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
		assert(scheduler_priority_time_slicing->current_mutex);

		uint32_t thread_priority = thread_control_block_get_priority(scheduler_priority_time_slicing->current_thread);
		thread_control_block_t* suspended_thread = (thread_control_block_t*) cyclic_iterator_pop(scheduler_priority_time_slicing->threads_iterators[thread_priority]);
		thread_control_block_set_stack_pointer(suspended_thread, SP_register);

		list_push_back(scheduler_priority_time_slicing->current_mutex->suspended_threads, suspended_thread);

		scheduler_priority_time_slicing->current_mutex = 0;

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

	return next_SP_register;
}


static void __scheduler_priority_time_slicing_deactivate_current_thread(void)
{
	uint32_t current_thread_priority = thread_control_block_get_priority(scheduler_priority_time_slicing_g->current_thread);
	thread_control_block_t* deactivated_thread = cyclic_iterator_pop(scheduler_priority_time_slicing_g->threads_iterators[current_thread_priority]);
	list_push_back(scheduler_priority_time_slicing_g->deactivated_threads, deactivated_thread);

	scheduler_priority_time_slicing_g->state = DEACTIVATE_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD;

	YIELD();

	assert(0);
}

static void __scheduler_priority_time_slicing_destroy_deactivated_threads(scheduler_t* scheduler)
{
	scheduler_priority_time_slicing_t* scheduler_priority_time_slicing = (scheduler_priority_time_slicing_t*) scheduler;

	__scheduler_priority_time_slicing_destroy_list_content(scheduler_priority_time_slicing->deactivated_threads);

}

static void __scheduler_priority_time_slicing_destroy_list_content(list_t* list)
{
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
}

mutex_t* __scheduler_priority_time_slicing_create_mutex(scheduler_t* scheduler)
{
	mutex_t* mutex = malloc(sizeof(*mutex));

	mutex->suspended_threads = list_create();
	mutex->suspended_threads_iterator = iterator_create(mutex->suspended_threads);
	mutex->is_locked = 0;

	mutex->mutex_lock = __mutex_priority_time_slicing_lock;
	mutex->mutex_unlock = __mutex_priority_time_slicing_unlock;
	mutex->mutex_destroy = __mutex_priority_time_slicing_destroy;

	return mutex;
}

static void __mutex_priority_time_slicing_lock(mutex_t* mutex)
{
	mutex->is_locked++;

	if (mutex->is_locked >= 2)
	{
		// TODO: add pointer to scheduler in mutex class
		scheduler_priority_time_slicing_t* scheduler_priority_time_slicing = scheduler_priority_time_slicing_g;
		scheduler_priority_time_slicing->state = SUSPEND_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD;
		scheduler_priority_time_slicing->current_mutex = mutex;

		YIELD();
	}
}

static void __mutex_priority_time_slicing_unlock(mutex_t* mutex)
{
	assert(mutex->is_locked);

	if (mutex->is_locked)
	{
		mutex->is_locked--;

		if (!list_is_empty(mutex->suspended_threads))
		{
			iterator_reset(mutex->suspended_threads_iterator);
			thread_control_block_t* resumed_thread = cyclic_iterator_pop(mutex->suspended_threads_iterator);
			// TODO: add pointer to scheduler in mutex class
			scheduler_priority_time_slicing_t* scheduler_priority_time_slicing = scheduler_priority_time_slicing_g;

			uint32_t thread_priority = thread_control_block_get_priority(scheduler_priority_time_slicing->current_thread);
			list_push_back(scheduler_priority_time_slicing->threads_lists[thread_priority], resumed_thread);
		}
	}
}

static void __mutex_priority_time_slicing_destroy(mutex_t* mutex)
{
	assert(list_is_empty(mutex->suspended_threads));
	list_destroy(mutex->suspended_threads);
	iterator_destroy(mutex->suspended_threads_iterator);

	free(mutex);
}

void  __scheduler_priority_time_slicing_block_thread(scheduler_t* scheduler, uint32_t delay)
{
	CRITICAL_PATH_ENTER();

	scheduler_priority_time_slicing_t* scheduler_priority_time_slicing = (scheduler_priority_time_slicing_t*) scheduler;
	thread_control_block_set_delay(scheduler_priority_time_slicing->current_thread, delay);
	scheduler_priority_time_slicing->state = BLOCK_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD;

	CRITICAL_PATH_EXIT();

	YIELD();
}
