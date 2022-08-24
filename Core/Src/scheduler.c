#include "scheduler.h"
#include "list.h"
#include "atomic.h"

struct scheduler
{
  list* threads;
  iterator* threads_iterator;
  uint32_t* main_thread_SP_register;
  uint32_t is_contex_to_save;
};

scheduler* scheduler_object = 0;

static void scheduler_remove_thread_from_ready_list(void);

scheduler* scheduler_create(const scheduler_attributes* scheduler_attributes_object)
{
  CRITICAL_PATH_ENTER();

  if (scheduler_object == 0)
  {
    scheduler_object = malloc(sizeof(*scheduler_object));

    scheduler_object->threads = list_create();
    scheduler_object->threads_iterator = iterator_create(scheduler_object->threads);
    scheduler_object->main_thread_SP_register = 0;
    scheduler_object->is_contex_to_save = 1;
  }

  CRITICAL_PATH_EXIT();

  return scheduler_object;
}

void scheduler_destroy(scheduler* scheduler_object)
{
  CRITICAL_PATH_ENTER();

  if (scheduler_object != 0)
  {
     free(scheduler_object);
  }

  CRITICAL_PATH_EXIT();
}

uint32_t scheduler_is_context_to_save(const scheduler* scheduler_object)
{
  return scheduler_object->is_contex_to_save;
}

void scheduler_add_thread(scheduler* scheduler_object, const thread_attributes* thread_attributes_object)
{
  CRITICAL_PATH_ENTER();

  thread_control_block* thread_control_block_object = thread_control_block_create(thread_attributes_object, scheduler_remove_thread_from_ready_list);
  list_push_back(scheduler_object->threads, thread_control_block_object);

  CRITICAL_PATH_EXIT();
}

uint32_t scheduler_choose_next_thread(scheduler* scheduler_object, uint32_t SP_register)
{
  if (scheduler_object->main_thread_SP_register == 0)
  {
    scheduler_object->main_thread_SP_register = (uint32_t*) SP_register;

    iterator_reset(scheduler_object->threads_iterator);
  }
  else if (scheduler_object->is_contex_to_save)
  {
    thread_control_block_set_stack_pointer((thread_control_block*)iteator_get_data(scheduler_object->threads_iterator), (uint32_t*) SP_register);

    cyclic_iterator_next(scheduler_object->threads_iterator);
  }


  uint32_t* next_thread_stack_pointer = thread_control_block_get_stack_pointer((thread_control_block*)iteator_get_data(scheduler_object->threads_iterator));

  if (next_thread_stack_pointer == 0)
  {
    next_thread_stack_pointer = scheduler_object->main_thread_SP_register;
  }

  return (uint32_t) next_thread_stack_pointer;
}

static void scheduler_remove_thread_from_ready_list(void)
{

  while(1);
}
