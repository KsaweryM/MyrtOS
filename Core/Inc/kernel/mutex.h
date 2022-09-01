#ifndef __MUTEX_H
#define __MUTEX_H

typedef struct mutex mutex;

struct mutex
{
	void* mutex_data;

	void (*mutex_lock)(mutex* mutex_object);
	void (*mutex_unlock)(mutex* mutex_object);
	void (*mutex_destroy)(mutex* mutex_object);
};

mutex* mutex_create(void);
void mutex_lock(mutex* mutex_object);
void mutex_unlock(mutex* mutex_object);
void mutex_destroy(mutex* mutex_object);

#endif
