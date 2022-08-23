#ifndef __THREAD_H
#define __THREAD_H

#include <stdint.h>

typedef struct thread_attributes
{
  void (*function)(void*);
  void* function_arguments;
  uint32_t stack_size;
  uint8_t thread_priority;
} thread_attributes;

typedef struct thread_control_block
{
  uint32_t* stack_pointer;
  uint8_t thread_priority;
  uint32_t allocated_memory_for_stack[];
} thread_control_block;

thread_control_block* thread_control_block_create(const thread_attributes* thread_attributes_object, void (*remove_thread_from_kernel_list)(void));
void thread_control_block_destroy(thread_control_block* thread_control_block_object);

#endif
