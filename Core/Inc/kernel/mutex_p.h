#ifndef __MUTEX_P_H
#define __MUTEX_P_H

#include <kernel/mutex.h>
#include <kernel/list.h>

struct mutex_t
{
	list_t* suspended_threads;
	iterator_t* suspended_threads_iterator;
	uint32_t is_locked;

	void (*mutex_lock)(mutex_t* mutex);
	void (*mutex_unlock)(mutex_t* mutex);
	void (*mutex_destroy)(mutex_t* mutex);
};

#endif
