#include <kernel/scheduler/scheduler_p.h>
#include <kernel/mutex_p.h>
#include <kernel/scheduler/scheduler_round_robin.h>
#include <kernel/list.h>
#include <kernel/atomic.h>
#include <kernel/kernel.h>
#include <kernel/blocker.h>
#include <stdlib.h>

typedef enum SCHEDULER_ROUND_ROBIN_STATE
{
	SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD,
	SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD,
	DEACTIVATE_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD,
	SUSPEND_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD,
	BLOCK_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD,
	NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD
} SCHEDULER_ROUND_ROBIN_STATE;

struct scheduler_round_robin_t
{
	scheduler_t scheduler;
  list_t* threads_list;
	list_t* deactivated_threads;
  iterator_t* threads_iterator;
  uint32_t* main_thread_SP_register;
	thread_control_block_t* current_thread;
	mutex_t* current_mutex;
	blocker_t* blocker;
	SCHEDULER_ROUND_ROBIN_STATE state;
};

scheduler_round_robin_t* scheduler_round_robin_g = 0;

static void __scheduler_round_robin_destroy(scheduler_t* scheduler);
static uint32_t* __scheduler_round_robin_choose_next_thread(scheduler_t* scheduler, uint32_t* SP_register);
static void __scheduler_round_robin_add_thread(scheduler_t* scheduler, const thread_attributes_t* thread_attributes);
static void __scheduler_round_robin_add_thread_control_block(scheduler_t* scheduler, thread_control_block_t* thread_control_block);
static mutex_t* __scheduler_round_robin_create_mutex(scheduler_t* scheduler);
void  __scheduler_round_robin_block_thread(scheduler_t* scheduler, uint32_t delay);

static void __scheduler_round_robin_deactivate_current_thread(void);
static void __scheduler_round_robin_destroy_deactivated_threads(scheduler_t* scheduler);

static void __mutex_round_robin_lock(mutex_t* mutex);
static void __mutex_round_robin_unlock(mutex_t* mutex);
static void __mutex_round_robin_destroy(mutex_t* mutex);

scheduler_round_robin_t* scheduler_round_robin_create()
{
	CRITICAL_PATH_ENTER();

	if (scheduler_round_robin_g == 0)
	{
		scheduler_round_robin_g = malloc(sizeof(*scheduler_round_robin_g));

		scheduler_round_robin_g->scheduler.scheduler_destroy = __scheduler_round_robin_destroy;
		scheduler_round_robin_g->scheduler.scheduler_add_thread = __scheduler_round_robin_add_thread;
		scheduler_round_robin_g->scheduler.scheduler_add_thread_control_block = __scheduler_round_robin_add_thread_control_block;
		scheduler_round_robin_g->scheduler.scheduler_choose_next_thread = __scheduler_round_robin_choose_next_thread;
		scheduler_round_robin_g->scheduler.scheduler_create_mutex = __scheduler_round_robin_create_mutex;
		scheduler_round_robin_g->scheduler.scheduler_block_thread = __scheduler_round_robin_block_thread;

		scheduler_round_robin_g->threads_list = list_create();
		scheduler_round_robin_g->deactivated_threads = list_create();
		scheduler_round_robin_g->threads_iterator = iterator_create(scheduler_round_robin_g->threads_list);
		scheduler_round_robin_g->main_thread_SP_register = 0;
		scheduler_round_robin_g->current_thread = 0;
		scheduler_round_robin_g->current_mutex = 0;
		scheduler_round_robin_g->blocker = blocker_create((scheduler_t*) scheduler_round_robin_g);

		scheduler_round_robin_g->state = SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}

	CRITICAL_PATH_EXIT();

	return scheduler_round_robin_g;
}

void __scheduler_round_robin_destroy(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	scheduler_round_robin_t* scheduler_round_robin = (scheduler_round_robin_t*) scheduler;

	list_t* scheduler_lists[] = {scheduler_round_robin->threads_list, scheduler_round_robin->deactivated_threads};
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

	iterator_destroy(scheduler_round_robin->threads_iterator);
	thread_control_block_destroy(scheduler_round_robin->current_thread);
	blocker_destroy(scheduler_round_robin->blocker);

	free(scheduler_round_robin);

	scheduler_round_robin_g = 0;

	CRITICAL_PATH_EXIT();
}

void __scheduler_round_robin_add_thread(scheduler_t* scheduler, const thread_attributes_t* thread_attributes)
{
	CRITICAL_PATH_ENTER();

	thread_control_block_t* thread_control_block = thread_control_block_create(thread_attributes, __scheduler_round_robin_deactivate_current_thread);
	scheduler_round_robin_t* scheduler_round_robin = (scheduler_round_robin_t*) scheduler;
	list_push_back(scheduler_round_robin->threads_list, thread_control_block);

	CRITICAL_PATH_EXIT();
}

void __scheduler_round_robin_add_thread_control_block(scheduler_t* scheduler, thread_control_block_t* thread_control_block)
{
	CRITICAL_PATH_ENTER();

	scheduler_round_robin_t* scheduler_round_robin = (scheduler_round_robin_t*) scheduler;
	list_push_back(scheduler_round_robin->threads_list, thread_control_block);

	CRITICAL_PATH_EXIT();
}

uint32_t* __scheduler_round_robin_choose_next_thread(scheduler_t* scheduler, uint32_t* SP_register)
{
	CRITICAL_PATH_ENTER();

	scheduler_round_robin_t* scheduler_round_robin = (scheduler_round_robin_t*) scheduler;

	if (scheduler_round_robin->state == SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD)
	{
		scheduler_round_robin->main_thread_SP_register = SP_register;

		iterator_reset(scheduler_round_robin->threads_iterator);

		scheduler_round_robin->current_thread = (thread_control_block_t*)iterator_get_data(scheduler_round_robin->threads_iterator);

		scheduler_round_robin->state = (scheduler_round_robin->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_round_robin->state == SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD)
	{
		thread_control_block_set_stack_pointer(scheduler_round_robin->current_thread, SP_register);

		cyclic_iterator_next(scheduler_round_robin->threads_iterator);
		scheduler_round_robin->current_thread = (thread_control_block_t*)iterator_get_data(scheduler_round_robin->threads_iterator);

		scheduler_round_robin->state = (scheduler_round_robin->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_round_robin->state == DEACTIVATE_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD)
	{
		scheduler_round_robin->current_thread = (thread_control_block_t*)iterator_get_data(scheduler_round_robin->threads_iterator);

		scheduler_round_robin->state = (scheduler_round_robin->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_round_robin->state == SUSPEND_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD)
	{
		assert(scheduler_round_robin->current_mutex);

		thread_control_block_t* suspended_thread = (thread_control_block_t*) cyclic_iterator_pop(scheduler_round_robin->threads_iterator);
		thread_control_block_set_stack_pointer(suspended_thread, SP_register);

		list_push_back(scheduler_round_robin->current_mutex->suspended_threads, suspended_thread);

		scheduler_round_robin->current_mutex = 0;

		scheduler_round_robin->current_thread = (thread_control_block_t*)iterator_get_data(scheduler_round_robin->threads_iterator);

		scheduler_round_robin->state = (scheduler_round_robin->current_thread == 0) ?
						NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_round_robin->state == BLOCK_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD)
	{
		thread_control_block_t* blocked_thread = (thread_control_block_t*) cyclic_iterator_pop(scheduler_round_robin->threads_iterator);
		thread_control_block_set_stack_pointer(blocked_thread, SP_register);

		blocker_block_thread(scheduler_round_robin->blocker, blocked_thread);

		scheduler_round_robin->current_thread = (thread_control_block_t*)iterator_get_data(scheduler_round_robin->threads_iterator);

		scheduler_round_robin->state = (scheduler_round_robin->current_thread == 0) ?
						NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}

	if (scheduler_round_robin->state == NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD)
	{
		__scheduler_round_robin_destroy_deactivated_threads(scheduler);

		scheduler_round_robin->state = SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}

	uint32_t* next_SP_register = ((scheduler_round_robin->current_thread == 0) ?
			scheduler_round_robin->main_thread_SP_register : thread_control_block_get_stack_pointer(scheduler_round_robin->current_thread));

	CRITICAL_PATH_EXIT();

	return next_SP_register;
}

static void __scheduler_round_robin_deactivate_current_thread(void)
{
	CRITICAL_PATH_ENTER();

	thread_control_block_t* deactivated_thread = cyclic_iterator_pop(scheduler_round_robin_g->threads_iterator);
	list_push_back(scheduler_round_robin_g->deactivated_threads, deactivated_thread);

	scheduler_round_robin_g->state = DEACTIVATE_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD;

	CRITICAL_PATH_EXIT();

	YIELD();

	assert(0);
}

static void __scheduler_round_robin_destroy_deactivated_threads(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	scheduler_round_robin_t* scheduler_round_robin = (scheduler_round_robin_t*) scheduler;

	iterator_t* deactivated_threads_iterator = iterator_create(scheduler_round_robin->deactivated_threads);

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

mutex_t* __scheduler_round_robin_create_mutex(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	mutex_t* mutex = malloc(sizeof(*mutex));

	mutex->suspended_threads = list_create();
	mutex->suspended_threads_iterator = iterator_create(mutex->suspended_threads);
	mutex->is_locked = 0;

	mutex->mutex_lock = __mutex_round_robin_lock;
	mutex->mutex_unlock = __mutex_round_robin_unlock;
	mutex->mutex_destroy = __mutex_round_robin_destroy;

	CRITICAL_PATH_EXIT();

	return mutex;
}

static void __mutex_round_robin_lock(mutex_t* mutex)
{
	CRITICAL_PATH_ENTER();

	mutex->is_locked++;

	bool is_mutex_locked = mutex->is_locked >= 2;

	if (is_mutex_locked)
	{
		scheduler_round_robin_t* scheduler = (scheduler_round_robin_t*) scheduler_round_robin_g;
		scheduler->state = SUSPEND_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD;
		scheduler->current_mutex = mutex;
	}

	CRITICAL_PATH_EXIT();

	if (is_mutex_locked)
	{
		YIELD();
	}
}

static void __mutex_round_robin_unlock(mutex_t* mutex)
{
	CRITICAL_PATH_ENTER();

	assert(mutex->is_locked);

	if (mutex->is_locked)
	{
		mutex->is_locked--;

		if (!list_is_empty(mutex->suspended_threads))
		{
			iterator_reset(mutex->suspended_threads_iterator);
			thread_control_block_t* resumed_thread = cyclic_iterator_pop(mutex->suspended_threads_iterator);
			scheduler_round_robin_t* scheduler_round_robin = (scheduler_round_robin_t*) scheduler_round_robin_g;
			list_push_back(scheduler_round_robin->threads_list, resumed_thread);
		}
	}

	CRITICAL_PATH_EXIT();
}

static void __mutex_round_robin_destroy(mutex_t* mutex)
{
	CRITICAL_PATH_ENTER();

	assert(list_is_empty(mutex->suspended_threads));
	list_destroy(mutex->suspended_threads);
	iterator_destroy(mutex->suspended_threads_iterator);

	free(mutex);

	CRITICAL_PATH_EXIT();
}

void  __scheduler_round_robin_block_thread(scheduler_t* scheduler, uint32_t delay)
{
	CRITICAL_PATH_ENTER();

	scheduler_round_robin_t* scheduler_round_robin = (scheduler_round_robin_t*) scheduler;
	thread_control_block_set_delay(scheduler_round_robin->current_thread, delay);
	scheduler_round_robin->state = BLOCK_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD;

	CRITICAL_PATH_EXIT();

	YIELD();
}

