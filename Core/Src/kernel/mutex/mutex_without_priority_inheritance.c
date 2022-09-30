#include <kernel/kernel.h>
#include <kernel/atomic.h>
#include <kernel/mutex/mutex.h>
#include <kernel/mutex/mutex_p.h>
#include <kernel/mutex/mutex_without_priority_inheritance.h>
#include <kernel/atomic.h>

struct mutex_without_priority_inheritance_t
{
	mutex_t mutex;
	uint32_t nr_locked_threads;
	list_t* suspended_threads;
	iterator_t* suspended_threads_iterator;
	scheduler_t* scheduler;
};

static void __mutex_without_priority_inheritance_destroy(mutex_t* mutex);
static void __mutex_without_priority_inheritance_lock(mutex_t* mutex);
static void __mutex_without_priority_inheritance_unlock(mutex_t* mutex);

mutex_without_priority_inheritance_t* mutex_without_priority_inheritance_create(scheduler_t* scheduler)
{
	CRITICAL_PATH_ENTER();

	mutex_without_priority_inheritance_t* mutex = malloc(sizeof(*mutex));

	mutex->mutex.mutex_destroy = __mutex_without_priority_inheritance_destroy;
	mutex->mutex.mutex_lock = __mutex_without_priority_inheritance_lock;
	mutex->mutex.mutex_unlock = __mutex_without_priority_inheritance_unlock;

	mutex->nr_locked_threads = 0;
	mutex->suspended_threads = list_create();
	mutex->suspended_threads_iterator = iterator_create(mutex->suspended_threads);
	mutex->scheduler = scheduler;

	CRITICAL_PATH_EXIT();

	return mutex;
}

static void __mutex_without_priority_inheritance_destroy(mutex_t* mutex)
{
	CRITICAL_PATH_ENTER();

	mutex_without_priority_inheritance_t* mutex_without_priority_inheritance = (mutex_without_priority_inheritance_t*) mutex;

	assert(list_is_empty(mutex_without_priority_inheritance->suspended_threads));
	list_destroy(mutex_without_priority_inheritance->suspended_threads);
	iterator_destroy(mutex_without_priority_inheritance->suspended_threads_iterator);

	CRITICAL_PATH_EXIT();
}

static void __mutex_without_priority_inheritance_lock(mutex_t* mutex)
{
	CRITICAL_PATH_ENTER();

	mutex_without_priority_inheritance_t* mutex_without_priority_inheritance = (mutex_without_priority_inheritance_t*) mutex;

	mutex_without_priority_inheritance->nr_locked_threads++;

	uint32_t is_mutex_overlocked = mutex_without_priority_inheritance->nr_locked_threads >= 2;

	if (is_mutex_overlocked)
	{
		scheduler_t* scheduler = mutex_without_priority_inheritance->scheduler;
		thread_control_block_t* thread_to_suspend = scheduler_get_current_thread(scheduler);
		list_push_back(mutex_without_priority_inheritance->suspended_threads, thread_to_suspend);
		scheduler_set_mutex_state(scheduler);
	}

	CRITICAL_PATH_EXIT();

	if (is_mutex_overlocked)
	{
		yield();
	}
}
static void __mutex_without_priority_inheritance_unlock(mutex_t* mutex)
{
	CRITICAL_PATH_ENTER();

	mutex_without_priority_inheritance_t* mutex_without_priority_inheritance = (mutex_without_priority_inheritance_t*) mutex;

	assert(mutex_without_priority_inheritance->nr_locked_threads > 0);

	mutex_without_priority_inheritance->nr_locked_threads--;

	if (mutex_without_priority_inheritance->nr_locked_threads > 0)
	{
		iterator_reset(mutex_without_priority_inheritance->suspended_threads_iterator);
		thread_control_block_t* resumed_thread = cyclic_iterator_pop(mutex_without_priority_inheritance->suspended_threads_iterator);
		scheduler_add_thread_control_block(mutex_without_priority_inheritance->scheduler, resumed_thread);
	}

	CRITICAL_PATH_EXIT();
}
