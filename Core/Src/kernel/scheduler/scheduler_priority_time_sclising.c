#include <kernel/scheduler/scheduler_priority_time_slicing.h>
#include "kernel/list.h"
#include <stdlib.h>

#define NUMBER_PRIORITIES 16

typedef struct scheduler_priority_time_slicing_data
{
	list* blocked_threads;
	list** threads_lists;
	iterator** threads_iterators;
	uint32_t* main_thread_SP_register;
	uint32_t is_contex_to_save;
	thread_control_block* current_thread;
} scheduler_priority_time_slicing_data;

void scheduler_priority_time_slicing_remove_thread_from_ready_list(void);
void scheduler_priority_time_slicing_thread(uint32_t seconds);

scheduler* scheduler_priority_time_slicing_create(const scheduler_attributes* scheduler_attributes_object)
{
	scheduler* scheduler_object = malloc(sizeof(*scheduler_object));

	scheduler_object->scheduler_methods = malloc(sizeof(*scheduler_object->scheduler_methods));

	scheduler_object->scheduler_methods->scheduler_destroy = scheduler_priority_time_slicing_destroy;
	scheduler_object->scheduler_methods->scheduler_is_context_to_save = scheduler_priority_time_slicing_is_context_to_save;
	scheduler_object->scheduler_methods->scheduler_choose_next_thread = scheduler_priority_time_slicing_choose_next_thread;
	scheduler_object->scheduler_methods->scheduler_add_thread = scheduler_priority_time_slicing_add_thread;

	scheduler_priority_time_slicing_data* scheduler_data = malloc(sizeof(*scheduler_data));

	scheduler_data->blocked_threads = list_create();
	scheduler_data->threads_lists = malloc(sizeof(*scheduler_data->threads_lists) * NUMBER_PRIORITIES);
	scheduler_data->threads_iterators = malloc(sizeof(*scheduler_data->threads_iterators) * NUMBER_PRIORITIES);

	for (uint32_t i = 0; i < NUMBER_PRIORITIES; i++)
	{
		scheduler_data->threads_lists[i] = list_create();
		scheduler_data->threads_iterators[i] = iterator_create(scheduler_data->threads_lists[i]);
	}

	scheduler_data->main_thread_SP_register = 0;
	scheduler_data->is_contex_to_save = 1;
	scheduler_data->current_thread = 0;

	scheduler_object->scheduler_data = scheduler_data;

	return scheduler_object;
}

void scheduler_priority_time_slicing_destroy(scheduler* scheduler_object)
{

}

uint32_t scheduler_priority_time_slicing_is_context_to_save(const scheduler* scheduler_object)
{
	return ((scheduler_priority_time_slicing_data*)scheduler_object->scheduler_data)->is_contex_to_save;
}

uint32_t scheduler_priority_time_slicing_choose_next_thread(scheduler* scheduler_object, uint32_t SP_register)
{
	scheduler_priority_time_slicing_data* scheduler_data = (scheduler_priority_time_slicing_data*) scheduler_object->scheduler_data;

	if (scheduler_data->main_thread_SP_register == 0)
	{
		scheduler_data->main_thread_SP_register = (uint32_t*) SP_register;

		for (uint32_t i = 0; i < NUMBER_PRIORITIES; i++)
		{
			iterator_reset(scheduler_data->threads_iterators[i]);
			cyclic_iterator_previous(scheduler_data->threads_iterators[i]);
		}
	}
	else if (scheduler_data->current_thread != 0)
	{
	  thread_control_block_set_stack_pointer(scheduler_data->current_thread, (uint32_t*) SP_register);
	}

	thread_control_block* next_thread = 0;

	for (int32_t i = NUMBER_PRIORITIES - 1; i >= 0; i--)
	{
		iterator* iterator_object = scheduler_data->threads_iterators[i];
		cyclic_iterator_next(scheduler_data->threads_iterators[i]);
		next_thread = (thread_control_block*)iteator_get_data(iterator_object);

		if (next_thread != 0)
		{
			break;
		}
	}

	scheduler_data->current_thread = next_thread;

	uint32_t* next_thread_stack_pointer = thread_control_block_get_stack_pointer(next_thread);

	if (next_thread_stack_pointer == 0)
	{
		next_thread_stack_pointer = scheduler_data->main_thread_SP_register;
		scheduler_data->main_thread_SP_register = 0;
	}

	return (uint32_t) next_thread_stack_pointer;
}

void scheduler_priority_time_slicing_add_thread(scheduler* scheduler_object, const thread_attributes* thread_attributes_object)
{
	thread_control_block* thread_control_block_object = thread_control_block_create(thread_attributes_object, scheduler_priority_time_slicing_remove_thread_from_ready_list);
	list_push_back(((scheduler_priority_time_slicing_data*) scheduler_object->scheduler_data)->threads_lists[thread_attributes_object->thread_priority], thread_control_block_object);
}

void scheduler_priority_time_slicing_remove_thread_from_ready_list(void)
{

  while(1);
}

void scheduler_priority_time_slicing_thread(uint32_t seconds)
{

}
