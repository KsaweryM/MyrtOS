#include <kernel/thread.h>
#include <string.h>
#include <stdlib.h>

struct thread_control_block
{
  uint32_t* stack_pointer;
  uint8_t thread_priority;
  uint32_t delay;
  uint32_t* allocated_memory_for_stack;
};

thread_control_block* thread_control_block_create(const thread_attributes* thread_attributes_object, void (*remove_thread_from_kernel_list)(void))
{
  register const uint32_t stack_size = thread_attributes_object->stack_size;
  thread_control_block* thread_control_block_object = malloc(sizeof(*thread_control_block_object));
  thread_control_block_object->thread_priority = thread_attributes_object->thread_priority;
  thread_control_block_object->delay = 0;
  thread_control_block_object->allocated_memory_for_stack = malloc(sizeof(*thread_control_block_object->allocated_memory_for_stack) * stack_size);
  memset(thread_control_block_object->allocated_memory_for_stack, 0, stack_size);

  // Register PSR
  thread_control_block_object->allocated_memory_for_stack[stack_size - 1] = (1U << 24);
  // Register PC
  thread_control_block_object->allocated_memory_for_stack[stack_size - 2]  = (int32_t) thread_attributes_object->function;
  // Register LR
  thread_control_block_object->allocated_memory_for_stack[stack_size - 3]  = (int32_t) remove_thread_from_kernel_list;
  // Register R12
  thread_control_block_object->allocated_memory_for_stack[stack_size - 4]  = 0x12121212;

  // Register R3, fourth 32-bit thread argument. It's unused.
  thread_control_block_object->allocated_memory_for_stack[stack_size - 5]  = 0x03030303;
  // Register R2, third 32-bit thread argument.  It's unused.
  thread_control_block_object->allocated_memory_for_stack[stack_size - 6]  = 0x02020202;
  // Register R1, second 32-bit thread argument. It's unused.
  thread_control_block_object->allocated_memory_for_stack[stack_size - 7]  = 0x01010101;
  // Register R0, first 32-bit thread argument. It's used to pass arguments to thread
  thread_control_block_object->allocated_memory_for_stack[stack_size - 8]  = (int32_t) thread_attributes_object->function_arguments;

  // Register R11
  thread_control_block_object->allocated_memory_for_stack[stack_size - 9]  = 0x11111111;
  // Register R10
  thread_control_block_object->allocated_memory_for_stack[stack_size - 10] = 0x10101010;
  // Register R9
  thread_control_block_object->allocated_memory_for_stack[stack_size - 11] = 0x09090909;
  // Register R8
  thread_control_block_object->allocated_memory_for_stack[stack_size - 12] = 0x08080808;
  // Register R7
  thread_control_block_object->allocated_memory_for_stack[stack_size - 13] = 0x07070707;
  // Register R6
  thread_control_block_object->allocated_memory_for_stack[stack_size - 14] = 0x06060606;
  // Register R5
  thread_control_block_object->allocated_memory_for_stack[stack_size - 15] = 0x05050505;
  // Register R4
  thread_control_block_object->allocated_memory_for_stack[stack_size - 16] = 0x04040404;

  thread_control_block_object->stack_pointer = &thread_control_block_object->allocated_memory_for_stack[stack_size - 16];

  return thread_control_block_object;
}

void thread_control_block_destroy(thread_control_block* thread_control_block_object)
{
	free(thread_control_block_object->allocated_memory_for_stack);
  free(thread_control_block_object);
}

uint32_t* thread_control_block_get_stack_pointer(const thread_control_block* thread_control_block_object)
{
  return thread_control_block_object != 0 ? thread_control_block_object->stack_pointer : 0;
}

void thread_control_block_set_stack_pointer(thread_control_block* thread_control_block_object, uint32_t* stack_pointer)
{
  if (thread_control_block_object != 0)
  {
    thread_control_block_object->stack_pointer = stack_pointer;
  }
}

uint32_t thread_control_block_get_delay(const thread_control_block* thread_control_block_object)
{
	return thread_control_block_object->delay;
}

void thread_control_block_set_delay(thread_control_block* thread_control_block_object, uint32_t delay)
{
	thread_control_block_object->delay = delay;
}
