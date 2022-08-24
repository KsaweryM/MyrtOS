#include <kernel/atomic.h>
#include <kernel/list.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/scheduler/scheduler_factory.h>

scheduler* scheduler_global_object = 0;

scheduler* scheduler_create(const scheduler_attributes* scheduler_attributes_object)
{
  CRITICAL_PATH_ENTER();

  if (scheduler_global_object == 0)
  {
    scheduler_global_object = scheduler_factory_create_scheduler(scheduler_attributes_object);
  }

  CRITICAL_PATH_EXIT();

  return scheduler_global_object;
}

void scheduler_destroy(scheduler* scheduler_object)
{
	scheduler_object->scheduler_methods->scheduler_destroy(scheduler_object);
}

uint32_t scheduler_is_context_to_save(const scheduler* scheduler_object)
{
	return scheduler_object->scheduler_methods->scheduler_is_context_to_save(scheduler_object);
}

void scheduler_add_thread(scheduler* scheduler_object, const thread_attributes* thread_attributes_object)
{
	scheduler_object->scheduler_methods->scheduler_add_thread(scheduler_object, thread_attributes_object);
}

uint32_t scheduler_choose_next_thread(scheduler* scheduler_object, uint32_t SP_register)
{
	return scheduler_object->scheduler_methods->scheduler_choose_next_thread(scheduler_object, SP_register);
}
