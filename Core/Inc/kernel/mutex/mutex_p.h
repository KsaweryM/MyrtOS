#ifndef __MUTEX_P_H
#define __MUTEX_P_H

#include <kernel/list.h>
#include <kernel/mutex/mutex.h>

struct mutex_t
{
	void (*mutex_lock)(mutex_t* mutex);
	void (*mutex_unlock)(mutex_t* mutex);
	void (*mutex_destroy)(mutex_t* mutex);
};

#endif
