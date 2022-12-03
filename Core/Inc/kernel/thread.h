#ifndef __THREAD_H
#define __THREAD_H

#include <stdint.h>

typedef struct thread_attributes_t
{
	char* thread_name;
	void (*function)(void*);
	void* function_arguments;
	uint32_t stack_size;
	uint8_t thread_priority;
} thread_attributes_t;

typedef struct thread_control_block_t thread_control_block_t;

thread_control_block_t* thread_control_block_create(const thread_attributes_t* thread_attributes, void (*thread_callback)(void));
void thread_control_block_destroy(thread_control_block_t* thread_control_block);

uint32_t thread_control_block_get_priority(const thread_control_block_t* thread_control_block);
void thread_control_block_set_priority(thread_control_block_t* thread_control_block, uint32_t priority);

uint32_t* thread_control_block_get_stack_pointer(const thread_control_block_t* thread_control_block);
void thread_control_block_set_stack_pointer(thread_control_block_t* thread_control_block, uint32_t* stack_pointer);

uint32_t thread_control_block_get_delay(const thread_control_block_t* thread_control_block);
void thread_control_block_set_delay(thread_control_block_t* thread_control_block, uint32_t delay);

char* thread_control_block_get_thread_name(const thread_control_block_t* thread_control_block);

#endif
