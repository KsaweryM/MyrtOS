#include "kernel/scheduler/scheduler_round_robin.h"
#include "kernel/list.h"
#include "kernel/atomic.h"
#include "kernel/kernel.h"
#include <stdlib.h>

scheduler* scheduler_round_robin_global_object = 0;

typedef struct scheduler_round_robin_data
{
  list* threads;
	list* threads_to_remove;
  iterator* threads_iterator;
  uint32_t* main_thread_SP_register;
  uint32_t is_contex_to_save; // useless, because we always should save context
  uint32_t interator_choose_next; // temporary
	thread_control_block* idle_task;
} scheduler_round_robin_data;

void scheduler_round_robin_remove_thread_from_ready_list(void);
void scheduler_round_robin_block_thread(uint32_t seconds);
void scheduler_round_robin_idle_task(void* args);
void scheduler_round_robin_create_idle_task(scheduler* scheduler_object);

scheduler* scheduler_round_robin_create(const scheduler_attributes* scheduler_attributes_object)
{
	CRITICAL_PATH_ENTER();

	if (scheduler_round_robin_global_object == 0)
	{
		scheduler* scheduler_object = malloc(sizeof(*scheduler_object));

		scheduler_object = malloc(sizeof(*scheduler_object));

		scheduler_object->scheduler_methods = malloc(sizeof(*scheduler_object->scheduler_methods));

		scheduler_object->scheduler_methods->scheduler_destroy = scheduler_round_robin_destroy;
		scheduler_object->scheduler_methods->scheduler_is_context_to_save = scheduler_round_robin_is_context_to_save;
		scheduler_object->scheduler_methods->scheduler_choose_next_thread = scheduler_round_robin_choose_next_thread;
		scheduler_object->scheduler_methods->scheduler_add_thread = scheduler_round_robin_add_thread;

		scheduler_round_robin_data* scheduler_data = malloc(sizeof(*scheduler_data));

		scheduler_data->threads = list_create();
		scheduler_data->threads_to_remove = list_create();
		scheduler_data->threads_iterator = iterator_create(scheduler_data->threads);
		scheduler_data->main_thread_SP_register = 0;
		scheduler_data->is_contex_to_save = 1;
		scheduler_data->interator_choose_next = 1;

		scheduler_object->scheduler_data = scheduler_data;

		scheduler_round_robin_create_idle_task(scheduler_object);

		scheduler_round_robin_global_object = scheduler_object;
	}

	CRITICAL_PATH_EXIT();

	return scheduler_round_robin_global_object;
}

void scheduler_round_robin_destroy(scheduler* scheduler_object)
{
	CRITICAL_PATH_ENTER();
	scheduler_round_robin_data* scheduler_data = (scheduler_round_robin_data*) scheduler_object->scheduler_data;

	iterator* iterator_object = iterator_create(scheduler_data->threads);

	while(!list_is_empty(scheduler_data->threads))
	{
		thread_control_block* thread_control_block_object = (thread_control_block*) iterator_pop(iterator_object);

		if (thread_control_block_object == 0)
		{
			break;
		}

		thread_control_block_destroy(thread_control_block_object);
	}

	iterator_destroy(iterator_object);

	iterator_object = iterator_create(scheduler_data->threads_to_remove);

	while(!list_is_empty(scheduler_data->threads_to_remove))
	{
		thread_control_block* thread_control_block_object = (thread_control_block*) iterator_pop(iterator_object);

		if (thread_control_block_object == 0)
		{
			break;
		}

		thread_control_block_destroy(thread_control_block_object);
	}

	iterator_destroy(iterator_object);


	list_destroy(scheduler_data->threads);
	list_destroy(scheduler_data->threads_to_remove);
	iterator_destroy(scheduler_data->threads_iterator);
	thread_control_block_destroy(scheduler_data->idle_task);

	free(scheduler_object->scheduler_data);
	free(scheduler_object->scheduler_methods);

	free(scheduler_object);

	CRITICAL_PATH_EXIT();
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
	else if (scheduler_data->interator_choose_next)
  {
    	thread_control_block_set_stack_pointer((thread_control_block*)iteator_get_data(scheduler_data->threads_iterator), (uint32_t*) SP_register);

    	cyclic_iterator_next(scheduler_data->threads_iterator);
  }

	scheduler_data->interator_choose_next = 1;

  uint32_t* next_thread_stack_pointer = thread_control_block_get_stack_pointer((thread_control_block*)iteator_get_data(scheduler_data->threads_iterator));

  if (next_thread_stack_pointer == 0)
  {

  	if (!list_is_empty(scheduler_data->threads_to_remove))
  	{
  		next_thread_stack_pointer = thread_control_block_get_stack_pointer(scheduler_data->idle_task);
  	}
  	else
  	{
      next_thread_stack_pointer = scheduler_data->main_thread_SP_register;
      scheduler_data->main_thread_SP_register = 0;
  	}
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
	CRITICAL_PATH_ENTER();

	scheduler* scheduler_object = scheduler_round_robin_global_object;
	scheduler_round_robin_data* scheduler_data = (scheduler_round_robin_data*) scheduler_object->scheduler_data;

	thread_control_block* thread_to_delete = iterator_pop(scheduler_data->threads_iterator);
	list_push_back(scheduler_data->threads_to_remove, thread_to_delete);
	scheduler_data->interator_choose_next = 0;

	CRITICAL_PATH_EXIT();

	thread_yield();

  while(1);
}

void scheduler_round_robin_block_thread(uint32_t seconds)
{

}

void scheduler_round_robin_idle_task(void* args)
{
	CRITICAL_PATH_ENTER();

	scheduler* scheduler_object = (scheduler*) args;
	scheduler_round_robin_data* scheduler_data = (scheduler_round_robin_data*) scheduler_object->scheduler_data;
	list* threads_to_remove = scheduler_data->threads_to_remove;

	iterator* iterator_object = iterator_create(threads_to_remove);
	iterator_reset(iterator_object);

	while (1)
	{
		thread_control_block* thread_control_block_object = (thread_control_block*) iterator_pop(iterator_object);

		if (thread_control_block_object == 0)
		{
			break;
		}

		thread_control_block_destroy(thread_control_block_object);
	}

	iterator_destroy(iterator_object);

	thread_yield();
	CRITICAL_PATH_EXIT();
}

void scheduler_round_robin_create_idle_task(scheduler* scheduler_object)
{
	thread_attributes thread_attributes_object = {
			.function = scheduler_round_robin_idle_task,
			.function_arguments = (void*) scheduler_object,
			.stack_size = 1000,
			.thread_priority = 0
	};

	thread_control_block* thread_control_block_object = thread_control_block_create(&thread_attributes_object, 0);

	((scheduler_round_robin_data*)scheduler_object->scheduler_data)->idle_task = thread_control_block_object;
}
