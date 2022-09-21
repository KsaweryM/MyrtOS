#include <kernel/atomic.h>
#include <kernel/list.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/scheduler/scheduler_p.h>

void scheduler_destroy(scheduler_t* scheduler)
{
	scheduler->scheduler_destroy(scheduler);
}

uint32_t* scheduler_choose_next_thread(scheduler_t* scheduler, uint32_t* SP_register)
{
	return scheduler->scheduler_choose_next_thread(scheduler, SP_register);
}

void scheduler_add_thread(scheduler_t* scheduler, const thread_attributes_t* thread_attributes)
{
	scheduler->scheduler_add_thread(scheduler, thread_attributes);
}

void scheduler_add_thread_control_block(scheduler_t* scheduler, thread_control_block_t* thread_control_block)
{
	scheduler->scheduler_add_thread_control_block(scheduler, thread_control_block);
}

void scheduler_block_thread(scheduler_t* scheduler, uint32_t delay)
{
	scheduler->scheduler_block_thread(scheduler, delay);
}

void scheduler_change_thread_priority(scheduler_t* scheduler, thread_control_block_t* thread_control_block, uint32_t priority)
{
	scheduler->scheduler_change_thread_priority(scheduler, thread_control_block, priority);
}

uint32_t scheduler_get_nr_priorities(scheduler_t* scheduler)
{
	return scheduler->scheduler_get_nr_priorities(scheduler);
}

thread_control_block_t* scheduler_get_current_thread(scheduler_t* scheduler)
{
	return scheduler->scheduler_get_current_thread(scheduler);
}

void scheduler_set_mutex_state(scheduler_t* scheduler)
{
	scheduler->scheduler_set_mutex_state(scheduler);
}
