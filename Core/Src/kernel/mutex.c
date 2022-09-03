#include <kernel/kernel.h>
#include <kernel/mutex.h>
#include <kernel/atomic.h>

mutex_t* mutex_create(void)
{
	mutex_t* mutex = 0;

  mutex = kernel_create_mutex(kernel_get_instance());

  return mutex;
}

void mutex_lock(mutex_t* mutex)
{
	mutex->mutex_lock(mutex);
}

void mutex_unlock(mutex_t* mutex)
{
	mutex->mutex_unlock(mutex);
}

void mutex_destroy(mutex_t* mutex)
{
	mutex->mutex_destroy(mutex);
}
