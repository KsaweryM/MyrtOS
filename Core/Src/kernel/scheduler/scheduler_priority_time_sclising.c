#include <kernel/scheduler/scheduler_priority_time_slicing.h>
#include "kernel/list.h"
#include "kernel/atomic.h"
#include "kernel/kernel.h"
#include <stdlib.h>

#define NUMBER_PRIORITIES 16

typedef enum SCHEDULER_PRIORITY_TIME_SLICING_STATE
{
	SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD,
	SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD,
	DEACTIVATE_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD,
	NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD

} SCHEDULER_PRIORITY_TIME_SLICING_STATE;

scheduler* scheduler_priority_time_slicing_global_object = 0;

typedef struct scheduler_priority_time_slicing_data
{
	list** threads_lists;
	list* suspended_threads;
	list* blocked_threads;
	list* deactivated_threads;
	iterator** threads_iterators;
	uint32_t* main_thread_SP_register;
	thread_control_block* current_thread;
	SCHEDULER_PRIORITY_TIME_SLICING_STATE state;
} scheduler_priority_time_slicing_data;

static void scheduler_priority_time_slicing_remove_thread_from_ready_list(void);
static void scheduler_priority_time_slicing_delete_deactivated_threads(scheduler* scheduler_object);

scheduler* scheduler_priority_time_slicing_create(const scheduler_attributes* scheduler_attributes_object)
{
	CRITICAL_PATH_ENTER();

	if (scheduler_priority_time_slicing_global_object == 0)
	{
		scheduler* scheduler_object = malloc(sizeof(*scheduler_object));

		scheduler_object->scheduler_methods = malloc(sizeof(*scheduler_object->scheduler_methods));

		scheduler_object->scheduler_methods->scheduler_destroy = scheduler_priority_time_slicing_destroy;
		scheduler_object->scheduler_methods->scheduler_is_context_to_save = scheduler_priority_time_slicing_is_context_to_save;
		scheduler_object->scheduler_methods->scheduler_add_thread = scheduler_priority_time_slicing_add_thread;
		scheduler_object->scheduler_methods->scheduler_choose_next_thread = scheduler_priority_time_slicing_choose_next_thread;

		scheduler_priority_time_slicing_data* scheduler_data = malloc(sizeof(*scheduler_data));

		scheduler_data->threads_lists = malloc(sizeof(*scheduler_data->threads_lists) * NUMBER_PRIORITIES);
		scheduler_data->suspended_threads = list_create();
		scheduler_data->blocked_threads = list_create();
		scheduler_data->deactivated_threads = list_create();
		scheduler_data->threads_iterators = malloc(sizeof(*scheduler_data->threads_iterators) * NUMBER_PRIORITIES);
		scheduler_data->main_thread_SP_register = 0;
		scheduler_data->current_thread = 0;
		scheduler_data->state = SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;

		for (uint32_t i = 0; i < NUMBER_PRIORITIES; i++)
		{
			scheduler_data->threads_lists[i] = list_create();
			scheduler_data->threads_iterators[i] = iterator_create(scheduler_data->threads_lists[i]);
		}

		scheduler_object->scheduler_data = scheduler_data;

		scheduler_priority_time_slicing_global_object = scheduler_object;
	}

	CRITICAL_PATH_ENTER();

	return scheduler_priority_time_slicing_global_object;
}

void scheduler_priority_time_slicing_destroy(scheduler* scheduler_object)
{

}

// TODO: delete
uint32_t scheduler_priority_time_slicing_is_context_to_save(const scheduler* scheduler_object)
{
	return 1;
}

void scheduler_priority_time_slicing_add_thread(scheduler* scheduler_object, const thread_attributes* thread_attributes_object)
{
	CRITICAL_PATH_ENTER();

	thread_control_block* thread_control_block_object = thread_control_block_create(thread_attributes_object, scheduler_priority_time_slicing_remove_thread_from_ready_list);
	list_push_back(((scheduler_priority_time_slicing_data*) scheduler_object->scheduler_data)->threads_lists[thread_attributes_object->thread_priority], thread_control_block_object);

	CRITICAL_PATH_EXIT();
}

// TODO: change type of SP_register to uint32_t*
uint32_t scheduler_priority_time_slicing_choose_next_thread(scheduler* scheduler_object, uint32_t SP_register)
{
	CRITICAL_PATH_ENTER();

	scheduler_priority_time_slicing_data* scheduler_data = (scheduler_priority_time_slicing_data*) scheduler_object->scheduler_data;

	if (scheduler_data->state == SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD)
	{
		scheduler_data->current_thread = 0;
		scheduler_data->main_thread_SP_register = (uint32_t*) SP_register;

		for (uint32_t i = 0; i < NUMBER_PRIORITIES; i++)
		{
			iterator_reset(scheduler_data->threads_iterators[i]);
		}

		int32_t iterator_index = NUMBER_PRIORITIES - 1;

		do
		{
			iterator* iterator_object = scheduler_data->threads_iterators[iterator_index];
			scheduler_data->current_thread = (thread_control_block*)iteator_get_data(iterator_object);
			iterator_index--;

		} while (scheduler_data->current_thread == 0 && iterator_index >= 0);

		scheduler_data->state = (scheduler_data->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_data->state == SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD)
	{
		thread_control_block_set_stack_pointer(scheduler_data->current_thread, (uint32_t*) SP_register);
		scheduler_data->current_thread = 0;

		int32_t iterator_index = NUMBER_PRIORITIES - 1;

		do
		{
			iterator* iterator_object = scheduler_data->threads_iterators[iterator_index];

			cyclic_iterator_next(iterator_object);
			scheduler_data->current_thread = (thread_control_block*)iteator_get_data(iterator_object);
			iterator_index--;

		} while (scheduler_data->current_thread == 0 && iterator_index >= 0);

		scheduler_data->state = (scheduler_data->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}
	else if (scheduler_data->state == DEACTIVATE_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD)
	{
		scheduler_data->current_thread = 0;

		int32_t iterator_index = NUMBER_PRIORITIES - 1;

		do
		{
			iterator* iterator_object = scheduler_data->threads_iterators[iterator_index];

			scheduler_data->current_thread = (thread_control_block*)iteator_get_data(iterator_object);
			iterator_index--;

		} while (scheduler_data->current_thread == 0 && iterator_index >= 0);

		scheduler_data->state = (scheduler_data->current_thread == 0) ?
				NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD : SAVE_CURRENT_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}

	if (scheduler_data->state == NO_THREADS_LEFT_SO_DELETE_DEACTIVATED_THREADS_ADD_CHOOSE_MAIN_THREAD)
	{
		scheduler_priority_time_slicing_delete_deactivated_threads(scheduler_object);

		scheduler_data->state = SAVE_MAIN_THREAD_SP_REGISTER_AND_CHOOSE_NEXT_THREAD;
	}


	uint32_t next_SP_register = (uint32_t) ((scheduler_data->current_thread == 0) ?
			scheduler_data->main_thread_SP_register : thread_control_block_get_stack_pointer(scheduler_data->current_thread));

	CRITICAL_PATH_EXIT();

	return next_SP_register;
}


static void scheduler_priority_time_slicing_remove_thread_from_ready_list(void)
{
	CRITICAL_PATH_ENTER();


	scheduler* scheduler_object = scheduler_priority_time_slicing_global_object;
	scheduler_priority_time_slicing_data* scheduler_data = (scheduler_priority_time_slicing_data*) scheduler_object->scheduler_data;

	uint32_t current_thread_priority = thread_control_block_get_priority(scheduler_data->current_thread);
	thread_control_block* deactivated_thread = iterator_pop(scheduler_data->threads_iterators[current_thread_priority]);
	list_push_back(scheduler_data->deactivated_threads, deactivated_thread);


	scheduler_data->state = DEACTIVATE_CURRENT_THREAD_AND_CHOOSE_NEXT_THREAD;

	CRITICAL_PATH_EXIT();

	thread_yield();

  while(1);
}

static void scheduler_priority_time_slicing_delete_deactivated_threads(scheduler* scheduler_object)
{
	CRITICAL_PATH_ENTER();

	scheduler_priority_time_slicing_data* scheduler_data = (scheduler_priority_time_slicing_data*) scheduler_object->scheduler_data;

	iterator* deactivated_threads_iterator = iterator_create(scheduler_data->deactivated_threads);

	while (1)
	{
		thread_control_block* thread_control_block_object = (thread_control_block*) iterator_pop(deactivated_threads_iterator);

		if (thread_control_block_object == 0)
		{
			break;
		}

		thread_control_block_destroy(thread_control_block_object);
	}

	iterator_destroy(deactivated_threads_iterator);

	CRITICAL_PATH_EXIT();
}
