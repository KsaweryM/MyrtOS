#include <kernel/atomic.h>
#include <kernel/kernel.h>
#include <kernel/list.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/mutex/mutex_with_priority_inheritance.h>
#include <kernel/mutex/mutex_without_priority_inheritance.h>
#include <assert.h>
#include <kernel/scheduler/scheduler_with_priority.h>
#include <kernel/scheduler/scheduler_without_priority.h>

#define CLKSOURCE		(1U << 2)
#define TICKINT			(1U << 1)
#define ENABLE			(1U << 0)

static void __system_timer_initialize_with_time_slicing(uint32_t CPU_frequency, uint32_t context_switch_period_in_milliseconds);
static void __system_timer_initialize_without_time_slicing(uint32_t CPU_frequency);
static uint32_t* __choose_next_thread(uint32_t* SP_register);
static void __kernel_block_thread(kernel_t* kernel, uint32_t delay);

struct kernel_t
{
  scheduler_t* scheduler;
  SCHEDULER_ALGORITHM scheduler_algorithm;
  uint32_t context_switch_period_in_milliseconds;

};

kernel_t* kernel_g = 0;
volatile uint32_t is_initialized = 0;

kernel_t* kernel_get_instance(void)
{
	return kernel_g;
}

kernel_t* kernel_create(const kernel_attributes_t* kernel_attributes)
{
  CRITICAL_PATH_ENTER();
  
  assert(!kernel_g);

  kernel_g = malloc(sizeof(*kernel_g));
  kernel_g->scheduler = 0;
  kernel_g->scheduler_algorithm = kernel_attributes->scheduler_algorithm;
  kernel_g->context_switch_period_in_milliseconds = 100; // default value

  switch (kernel_g->scheduler_algorithm)
  {
  case ROUND_ROBIN_SCHEDULING:
  	kernel_g->scheduler = (scheduler_t*) scheduler_without_priority_create();
  	break;
  case PRIORITIZED_PREEMPTIVE_SCHEDULING_WITH_TIME_SLICING:
  	kernel_g->scheduler = (scheduler_t*) scheduler_with_priority_create();
  	break;
  case PRIORITIZED_PREEMPTIVE_SCHEDULING_WITHOUT_TIME_SLICING:
  	kernel_g->scheduler = (scheduler_t*) scheduler_with_priority_create();
  	break;
  case COOPERATIVE_SCHEDULING:
  	kernel_g->scheduler = (scheduler_t*) scheduler_without_priority_create();
  	break;
  }

  assert(kernel_g->scheduler);

  is_initialized = 1;

  CRITICAL_PATH_EXIT();

  return kernel_g;
}

//TODO: go back to main thread and disable SysTick Interrupt
void kernel_destroy(kernel_t* kernel)
{
  CRITICAL_PATH_ENTER();

  SysTick->CTRL &= !TICKINT;

  SysTick->CTRL &= !ENABLE;

  __NVIC_ClearPendingIRQ(SysTick_IRQn);

  assert(kernel == kernel_g);
  assert(kernel_g);
  
  scheduler_destroy(kernel_g->scheduler);

  free(kernel_g);

  kernel_g = 0;

  is_initialized = 0;

  CRITICAL_PATH_EXIT();
}

void kernel_launch(const kernel_t* kernel)
{
  uint32_t CPU_frequency = get_CPU_frequency();

  scheduler_launch(kernel->scheduler);

  if (kernel->scheduler_algorithm == ROUND_ROBIN_SCHEDULING || kernel->scheduler_algorithm == PRIORITIZED_PREEMPTIVE_SCHEDULING_WITH_TIME_SLICING)
  {
  	__system_timer_initialize_with_time_slicing(CPU_frequency, kernel->context_switch_period_in_milliseconds);
  }
  else if (kernel->scheduler_algorithm == PRIORITIZED_PREEMPTIVE_SCHEDULING_WITHOUT_TIME_SLICING || kernel->scheduler_algorithm == COOPERATIVE_SCHEDULING)
  {
  	__system_timer_initialize_without_time_slicing(CPU_frequency);
  }
  else
  {
  	assert(0);
  }

  yield();
}

void kernel_add_thread(kernel_t* kernel, const thread_attributes_t* thread_attributes)
{
  CRITICAL_PATH_ENTER();

  scheduler_add_thread(kernel->scheduler, thread_attributes);

  CRITICAL_PATH_EXIT();
}

mutex_t* kernel_create_mutex(kernel_t* kernel)
{
  CRITICAL_PATH_ENTER();

  mutex_t* mutex = 0;

  if (kernel->scheduler_algorithm == ROUND_ROBIN_SCHEDULING
  		|| kernel->scheduler_algorithm == COOPERATIVE_SCHEDULING)
  {
  	mutex = (mutex_t*) mutex_without_priority_inheritance_create(kernel->scheduler);
  }
  else if (kernel->scheduler_algorithm == PRIORITIZED_PREEMPTIVE_SCHEDULING_WITH_TIME_SLICING
  		|| kernel->scheduler_algorithm == PRIORITIZED_PREEMPTIVE_SCHEDULING_WITHOUT_TIME_SLICING)
  {
  	mutex = (mutex_t*) mutex_with_priority_inheritance_create(kernel->scheduler);
  }

  assert(mutex);

  CRITICAL_PATH_EXIT();

  return mutex;
}

void __kernel_block_thread(kernel_t* kernel, uint32_t delay)
{
	scheduler_block_thread(kernel->scheduler, delay);
}

__attribute__((naked)) void SysTick_Handler(void)
{
  __asm("CPSID  I");

  __asm("save_contex: PUSH {R4-R11}");

  __asm("MOV R0, SP");

  __asm("PUSH {LR}");

  __asm("BL __choose_next_thread");

  __asm("POP {LR}");

  __asm("MOV SP, R0");

  __asm("POP {R4-R11}");

  __asm("CPSIE I");

  __asm("BX LR");
}

__attribute__((unused)) static uint32_t* __choose_next_thread(uint32_t* SP_register)
{
	if (!is_initialized)
	{
		assert(is_initialized);
	}

  return scheduler_choose_next_thread(kernel_g->scheduler, SP_register);
}

static void __system_timer_initialize_with_time_slicing(uint32_t CPU_frequency, uint32_t context_switch_period_in_milliseconds)
{
  /*
   kernel uses a 24-bit system timer SysTick.
   SysTick can be manipulated be 4 registers.

   SYST_CSR - SysTick Control and Status Register (0xE000E010)

   SYST_RVR - SysTick Reload Value Register (0xE000E014)

   SYST_CVR -  SysTick Current Value Register (0xE000E018)

   SYST_CALIB - SysTick Calibration Value Register (0xE000E01C)

   _____________________________________________
   SYST_CSR
     Bits             Name              Function
     [16]             COUNTFLAG         COUNTFLAG Returns 1 if timer counted to 0 since last time this was read.

     [2]              CLKSOURCE         Indicates the clock source:
                                        0 = external clock
                                        1 = processor clock

     [1]              TICKINT           Enables SysTick exception request:
                                        0 = counting down to zero does not assert the SysTick exception request
                                        1 = counting down to zero asserts the SysTick exception request.
                                        Software can use COUNTFLAG to determine if SysTick has ever counted to zero

     [0]              ENABLE            Enables the counter:
                                        0 = counter disabled
                                        1 = counter enabled.
   _____________________________________________
   SYST_RVR
     Bits             Name              Function
     [23:0]           RELOAD            Value to load into the SYST_CVR register when the counter is enabled and when it reaches 0

    _____________________________________________
   SYST_CVR
     Bits             Name              Function
     [23:0]           CURRENT           Reads return the current value of the SysTick counter

   */

  register const uint32_t ticks_per_millisecond = CPU_frequency / 1000;

  // TOOD: get these attributes from user
  //register const uint32_t context_switch_period_in_milliseconds = 100;
  register const uint32_t kernel_priority = 15;
  register const uint32_t reload_value = ticks_per_millisecond * context_switch_period_in_milliseconds - 1;

  assert (reload_value < (1U << 24));

  // Reset SYST_CSR register
  SysTick->CTRL = 0;

  // Reset SYST_CVR register
  SysTick->VAL = 0;

  // Set reload value in SYST_RVR register
  SysTick->LOAD = reload_value;

  // Set kernel priority
  NVIC_SetPriority(SysTick_IRQn, kernel_priority);

  // Indicates the clock source by setting appropriate bit in SYST_CSR register
  SysTick->CTRL |= CLKSOURCE;

  // Enables SysTick exception request by setting appropriate bit in SYST_CSR register
  SysTick->CTRL |= TICKINT;

  // Enables the counter by setting appropriate bit in SYST_CSR register
  SysTick->CTRL |= ENABLE;
}

static void __system_timer_initialize_without_time_slicing(uint32_t CPU_frequency)
{
  register const uint32_t kernel_priority = 15;

  // Reset SYST_CSR register
  SysTick->CTRL = 0;

  // Reset SYST_CVR register
  SysTick->VAL = 0;

  // Set kernel priority
  NVIC_SetPriority(SysTick_IRQn, kernel_priority);

  // Indicates the clock source by setting appropriate bit in SYST_CSR register
  SysTick->CTRL |= CLKSOURCE;

  // Enables SysTick exception request by setting appropriate bit in SYST_CSR register
  SysTick->CTRL |= TICKINT;
}

void delay(uint32_t delay)
{
	assert(kernel_g);
	__kernel_block_thread(kernel_g, delay);
}

uint32_t get_CPU_frequency()
{
  // TODO: get current CPU clock frequency
  return 4000000;
}

void kernel_change_context_switch_period_in_milliseconds(kernel_t* kernel, uint32_t period)
{
	kernel->context_switch_period_in_milliseconds = period;
}

void kernel_destroy_deactivated_threads(void)
{
	assert(kernel_g);
	scheduler_destroy_deactivated_threads(kernel_g->scheduler);
}
