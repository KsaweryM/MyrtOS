#include <kernel/atomic.h>
#include <kernel/kernel.h>
#include <kernel/list.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/scheduler/scheduler_round_robin.h>
#include <kernel/scheduler/scheduler_priority_time_slicing.h>
#include <assert.h>

#define INTCTRL			(*((volatile uint32_t *)0xE000ED04))
#define PENDSTET		(1U << 26)
#define CLKSOURCE		(1U << 2)
#define TICKINT			(1U << 1)
#define ENABLE			(1U << 0)

static void __system_timer_initialize(uint32_t CPU_frequency);
static uint32_t __get_CPU_frequency();
static uint32_t* __choose_next_thread(uint32_t* SP_register);

struct kernel_t
{
  scheduler_t* scheduler;
};

kernel_t* kernel_g = 0;

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

  switch (kernel_attributes->scheduler_algorithm)
  {
  case ROUND_ROBIN_SCHEDULING:
  	kernel_g->scheduler = (scheduler_t*) scheduler_round_robin_create();
  	break;
  case PRIORITIZED_PREEMPTIVE_SCHEDULING_WITH_TIME_SLICING:
  	kernel_g->scheduler = (scheduler_t*) scheduler_priority_time_slicing_create();
  	break;
  case PRIORITIZED_PREEMPTIVE_SCHEDULING_WITHOUT_TIME_SLICING:
  	break;
  case COOPERATIVE_SCHEDULING:
  	break;
  }

	assert(kernel_g->scheduler);

  CRITICAL_PATH_EXIT();

  return kernel_g;
}

//TODO: go back to main thread and disable SysTick Interrupt
void kernel_destroy(kernel_t* kernel)
{
  CRITICAL_PATH_ENTER();

  assert(kernel == kernel_g);
  assert(kernel_g);
  
  scheduler_destroy(kernel_g->scheduler);

  free(kernel_g);

  kernel_g = 0;

  // disable sysTick
  SysTick->CTRL &= !TICKINT;

  // remove pend state
  INTCTRL &= !PENDSTET;

  CRITICAL_PATH_EXIT();
}

void kernel_launch(const kernel_t* kernel)
{
  uint32_t CPU_frequency = __get_CPU_frequency();

  __system_timer_initialize(CPU_frequency);

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

  assert(kernel);

  mutex_t* mutex = scheduler_create_mutex(kernel->scheduler);

  CRITICAL_PATH_EXIT();

  return mutex;
}

void yield(void)
{
  __disable_irq();
  SysTick->VAL = 0;

  INTCTRL = PENDSTET;
  __enable_irq();

  while (INTCTRL & PENDSTET);
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
  return scheduler_choose_next_thread(kernel_g->scheduler, SP_register);
}

static void __system_timer_initialize(uint32_t CPU_frequency)
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
  register const uint32_t context_switch_period_in_milliseconds = 100;
  register const uint32_t kernel_priority = 15;

  // Reset SYST_CSR register
  SysTick->CTRL = 0;

  // Reset SYST_CVR register
  SysTick->VAL = 0;

  // Set reload value in SYST_RVR register
  SysTick->LOAD = ticks_per_millisecond * context_switch_period_in_milliseconds - 1;

  // Set kernel priority
  NVIC_SetPriority(SysTick_IRQn, kernel_priority);

  // Indicates the clock source by setting appropriate bit in SYST_CSR register
  SysTick->CTRL |= CLKSOURCE;

  // Enables SysTick exception request by setting appropriate bit in SYST_CSR register
  SysTick->CTRL |= TICKINT;

  // Enables the counter by setting appropriate bit in SYST_CSR register
  SysTick->CTRL |= ENABLE;
}

static uint32_t __get_CPU_frequency()
{
  // TODO: get current CPU clock frequency
  return 4000000;
}
