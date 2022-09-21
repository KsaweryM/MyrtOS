#include <kernel/kernel.h>
#include <kernel/atomic.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/mutex/mutex.h>
#include <kernel/mutex/mutex_p.h>
#include <kernel/mutex/mutex_with_priority_inheritance.h>
#include <kernel/thread.h>
#include <kernel/list.h>

struct mutex_with_priority_inheritance_t
{
	mutex_t mutex;
	uint32_t nr_locks;
	uint32_t nr_priorities;
	list_t** suspended_threads_lists;
	iterator_t** suspended_threads_iterators;
	thread_control_block_t* mutex_owner;
	uint32_t mutex_owner_default_priority;
	scheduler_t* scheduler;
};

static void __mutex_with_priority_inheritance_destroy(mutex_t* mutex);
static void __mutex_with_priority_inheritance_lock(mutex_t* mutex);
static void __mutex_with_priority_inheritance_unlock(mutex_t* mutex);

mutex_with_priority_inheritance_t* mutex_with_priority_inheritance_create(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	mutex_with_priority_inheritance_t* mutex = malloc(sizeof(*mutex));

	mutex->mutex.mutex_destroy = __mutex_with_priority_inheritance_destroy;
	mutex->mutex.mutex_lock = __mutex_with_priority_inheritance_lock;
	mutex->mutex.mutex_unlock = __mutex_with_priority_inheritance_unlock;

	mutex->nr_locks = 0;
	mutex->nr_priorities = scheduler_get_nr_priorities(scheduler);
	mutex->scheduler = scheduler;

	mutex->mutex_owner = 0;
	mutex->mutex_owner_default_priority = 0;

	mutex->suspended_threads_lists = malloc(sizeof(*mutex->suspended_threads_lists) * mutex->nr_priorities);
	mutex->suspended_threads_iterators = malloc(sizeof(*mutex->suspended_threads_iterators) * mutex->nr_priorities);

	for (uint32_t i = 0; i < mutex->nr_priorities; i++)
	{
		mutex->suspended_threads_lists[i] = list_create();
		assert(list_is_empty(mutex->suspended_threads_lists[i]));

		for (uint32_t j = 0; j < i; j++)
		{
			assert(list_is_empty(mutex->suspended_threads_lists[j]));
		}

		mutex->suspended_threads_iterators[i] = iterator_create(mutex->suspended_threads_lists[i]);
		assert(list_is_empty(mutex->suspended_threads_lists[i]));

		for (uint32_t j = 0; j < i; j++)
		{
			assert(list_is_empty(mutex->suspended_threads_lists[j]));
		}
	}

	for (uint32_t i = 0; i < mutex->nr_priorities; i++)
	{
		assert(list_is_empty(mutex->suspended_threads_lists[i]));
	}

	CRITICAL_PATH_EXIT();

	return mutex;
}

static void __mutex_with_priority_inheritance_destroy(mutex_t* mutex)
{
	CRITICAL_PATH_ENTER();

	mutex_with_priority_inheritance_t* mutex_with_priority_inheritance = (mutex_with_priority_inheritance_t*) mutex;

	for (uint32_t i = 0; i < mutex_with_priority_inheritance->nr_priorities; i++)
	{
		assert(list_is_empty(mutex_with_priority_inheritance->suspended_threads_lists[i]));

		iterator_destroy(mutex_with_priority_inheritance->suspended_threads_iterators[i]);
		list_destroy(mutex_with_priority_inheritance->suspended_threads_lists[i]);
	}

	free(mutex_with_priority_inheritance->suspended_threads_iterators);
	free(mutex_with_priority_inheritance->suspended_threads_lists);

	free(mutex_with_priority_inheritance);

	CRITICAL_PATH_EXIT();
}

static void __mutex_with_priority_inheritance_lock(mutex_t* mutex)
{
	critical_path_depth_test();

	CRITICAL_PATH_ENTER();

	mutex_with_priority_inheritance_t* mutex_with_priority_inheritance = (mutex_with_priority_inheritance_t*) mutex;

	mutex_with_priority_inheritance->nr_locks++;

	if (mutex_with_priority_inheritance->nr_locks == 1)
	{
		mutex_with_priority_inheritance->mutex_owner = scheduler_get_current_thread(mutex_with_priority_inheritance->scheduler);
		mutex_with_priority_inheritance->mutex_owner_default_priority = thread_control_block_get_priority(mutex_with_priority_inheritance->mutex_owner);
	}
	else
	{
		thread_control_block_t* suspended_thread = scheduler_get_current_thread(mutex_with_priority_inheritance->scheduler);

		uint32_t mutex_owner_priority = thread_control_block_get_priority(mutex_with_priority_inheritance->mutex_owner);
		uint32_t suspended_thread_priority = thread_control_block_get_priority(suspended_thread);

		if (suspended_thread_priority > mutex_owner_priority)
		{
			scheduler_change_thread_priority(mutex_with_priority_inheritance->scheduler, mutex_with_priority_inheritance->mutex_owner, suspended_thread_priority);
		}

		assert(suspended_thread_priority < mutex_with_priority_inheritance->nr_priorities);
		list_push_back(mutex_with_priority_inheritance->suspended_threads_lists[suspended_thread_priority], suspended_thread);
		scheduler_set_mutex_state(mutex_with_priority_inheritance->scheduler);
	}

	uint32_t is_mutex_overlocked = mutex_with_priority_inheritance->nr_locks >= 2;

	CRITICAL_PATH_EXIT();

	if (is_mutex_overlocked)
	{
		YIELD();
	}
}
static void __mutex_with_priority_inheritance_unlock(mutex_t* mutex)
{
	CRITICAL_PATH_ENTER();

	mutex_with_priority_inheritance_t* mutex_with_priority_inheritance = (mutex_with_priority_inheritance_t*) mutex;

	thread_control_block_t* resumed_thread = mutex_with_priority_inheritance->mutex_owner;

	assert(resumed_thread == scheduler_get_current_thread(mutex_with_priority_inheritance->scheduler)&& mutex_with_priority_inheritance->nr_locks >= 1);
	mutex_with_priority_inheritance->nr_locks--;

	if (thread_control_block_get_priority(resumed_thread) != mutex_with_priority_inheritance->mutex_owner_default_priority)
	{
		scheduler_change_thread_priority(mutex_with_priority_inheritance->scheduler, resumed_thread, mutex_with_priority_inheritance->mutex_owner_default_priority);
	}

	mutex_with_priority_inheritance->mutex_owner = 0;
	mutex_with_priority_inheritance->mutex_owner_default_priority = 0;

	if (mutex_with_priority_inheritance->nr_locks > 0)
	{
		int32_t iterator_index = mutex_with_priority_inheritance->nr_priorities - 1;

		do
		{
			iterator_t* iterator = mutex_with_priority_inheritance->suspended_threads_iterators[iterator_index];

			cyclic_iterator_next(iterator);
			mutex_with_priority_inheritance->mutex_owner = (thread_control_block_t*)iterator_pop(iterator);
			iterator_index--;

		} while (mutex_with_priority_inheritance->mutex_owner == 0 && iterator_index >= 0);

		assert(mutex_with_priority_inheritance->mutex_owner);

		mutex_with_priority_inheritance->mutex_owner_default_priority = thread_control_block_get_priority(mutex_with_priority_inheritance->mutex_owner);

		scheduler_add_thread_control_block((scheduler_t*) mutex_with_priority_inheritance->scheduler, mutex_with_priority_inheritance->mutex_owner);
	}

	CRITICAL_PATH_EXIT();
}
