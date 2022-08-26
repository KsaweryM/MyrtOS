#include "kernel/scheduler/scheduler_round_robin.h"
#include "kernel/list.h"
#include <stdlib.h>

typedef struct scheduler_round_robin_data
{
  list* threads;
  iterator* threads_iterator;
  uint32_t* main_thread_SP_register;
  uint32_t is_contex_to_save;
} scheduler_round_robin_data;

void scheduler_round_robin_remove_thread_from_ready_list(void);
void scheduler_round_robin_block_thread(uint32_t seconds);

scheduler* scheduler_round_robin_create(const scheduler_attributes* scheduler_attributes_object)
{
	scheduler* scheduler_object = malloc(sizeof(*scheduler_object));

	scheduler_object->scheduler_methods = malloc(sizeof(*scheduler_object->scheduler_methods));

	scheduler_object->scheduler_methods->scheduler_destroy = scheduler_round_robin_destroy;
	scheduler_object->scheduler_methods->scheduler_is_context_to_save = scheduler_round_robin_is_context_to_save;
	scheduler_object->scheduler_methods->scheduler_choose_next_thread = scheduler_round_robin_choose_next_thread;
	scheduler_object->scheduler_methods->scheduler_add_thread = scheduler_round_robin_add_thread;

	scheduler_round_robin_data* scheduler_data = malloc(sizeof(*scheduler_data));

	scheduler_data->threads = list_create();
	scheduler_data->threads_iterator = iterator_create(scheduler_data->threads);
	scheduler_data->main_thread_SP_register = 0;
	scheduler_data->is_contex_to_save = 1;

	scheduler_object->scheduler_data = scheduler_data;

	return scheduler_object;
}

void scheduler_round_robin_destroy(scheduler* scheduler_object)
{

}

uint32_t scheduler_round_robin_is_context_to_save(const scheduler* scheduler_object)
{
	return ((scheduler_round_robin_data*)scheduler_object->scheduler_data)->is_contex_to_save;
}

uint32_t scheduler_round_robin_choose_next_thread(scheduler* scheduler_object, uint32_t SP_register)
{
	scheduler_round_robin_data* scheduler_data = (scheduler_round_robin_data*) scheduler_object->scheduler_data;

	if (scheduler_data->main_thread_SP_register == 0)
	{
		scheduler_data->main_thread_SP_register = (uint32_t*) SP_register;

		iterator_reset(scheduler_data->threads_iterator);
	}
	else if (scheduler_data->is_contex_to_save)
  {
    thread_control_block_set_stack_pointer((thread_control_block*)iteator_get_data(scheduler_data->threads_iterator), (uint32_t*) SP_register);

    cyclic_iterator_next(scheduler_data->threads_iterator);
  }

  uint32_t* next_thread_stack_pointer = thread_control_block_get_stack_pointer((thread_control_block*)iteator_get_data(scheduler_data->threads_iterator));

  if (next_thread_stack_pointer == 0)
  {
    next_thread_stack_pointer = scheduler_data->main_thread_SP_register;
    scheduler_data->main_thread_SP_register = 0;
  }

  return (uint32_t) next_thread_stack_pointer;
}

void scheduler_round_robin_add_thread(scheduler* scheduler_object, const thread_attributes* thread_attributes_object)
{
	thread_control_block* thread_control_block_object = thread_control_block_create(thread_attributes_object, scheduler_round_robin_remove_thread_from_ready_list);
	list_push_back(((scheduler_round_robin_data*) scheduler_object->scheduler_data)->threads, thread_control_block_object);
}

void scheduler_round_robin_remove_thread_from_ready_list(void)
{

  while(1);
}

void scheduler_round_robin_block_thread(uint32_t seconds)
{

}
