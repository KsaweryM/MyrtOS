#include <kernel/kernel.h>
#include <kernel/mutex.h>
#include <kernel/atomic.h>

mutex* mutex_create(void)
{
	mutex* mutex_object = 0;

  CRITICAL_PATH_ENTER();

  mutex_object = kernel_create_mutex(kernel_get_instance());

  CRITICAL_PATH_EXIT();

  return mutex_object;
}

void mutex_lock(mutex* mutex_object)
{
  CRITICAL_PATH_ENTER();

	mutex_object->mutex_lock(mutex_object);

  CRITICAL_PATH_EXIT();
}

void mutex_unlock(mutex* mutex_object)
{
  CRITICAL_PATH_ENTER();

	mutex_object->mutex_unlock(mutex_object);

  CRITICAL_PATH_EXIT();
}

void mutex_destroy(mutex* mutex_object)
{
  CRITICAL_PATH_ENTER();

	mutex_object->mutex_destroy(mutex_object);

  CRITICAL_PATH_EXIT();
}
