#include "kernel.h"
#include "atomic.h"
#include "list.h"

list* threads = 0;
volatile thread_control_block* current_thread = 0;
iterator* threads_iterator = 0;
uint32_t* main_thread_SP_register = 0;

uint32_t* choose_next_thread(uint32_t* SP);
static void remove_thread_from_kernel_list(void);
static void system_timer_initialize(uint32_t CPU_frequency);
static void delay_timer_initialize(uint32_t CPU_frequency);
static uint32_t get_CPU_frequency();

void kernel_create(void)
{
  threads = list_create();
}

void kernel_destroy(void)
{

}

void kernel_launch(void)
{
  uint32_t CPU_frequency = get_CPU_frequency();

  system_timer_initialize(CPU_frequency);
  delay_timer_initialize(CPU_frequency);

  threads_iterator = iterator_create(threads);
  thread_yield();
}

void kernel_suspend(void)
{

}

void kernel_resume(void)
{

}

void kernel_add_thread(const thread_attributes* thread_attributes_object)
{
  thread_control_block* thread_control_block_object = thread_control_block_create(thread_attributes_object, remove_thread_from_kernel_list);
  list_push_back(threads, thread_control_block_object);
}

void thread_delay(uint32_t seconds)
{
}

#define INTCTRL (*((volatile uint32_t *)0xE000ED04))
#define PENDSTET  (1U<<26)

void thread_yield(void)
{
  __disable_irq();
  /*Clear SysTick Current value register*/
  SysTick->VAL = 0;

  /*Trigger SysTick*/
  INTCTRL = PENDSTET;
  __enable_irq();
}

__attribute__((naked)) void SysTick_Handler(void)
{
  // disable interrupts
  __asm("CPSID  I");

  //TODO: if current_thread != 0, save context
  __asm("LDR R0, =main_thread_SP_register");

  __asm("LDR R0, [R0]");

  __asm("CMP R0, #0");

  __asm("BEQ save");

  __asm("LDR R0, =current_thread");

  __asm("LDR R0, [R0]");

  __asm("CMP R0, #0");

  __asm("BEQ skip");

  __asm("save: PUSH {R4-R11}");

  __asm("skip: MOV R0, SP");

  __asm("PUSH {LR}");

  __asm("BL choose_next_thread");

  __asm("POP {LR}");

  __asm("MOV SP, R0");

  __asm("POP {R4-R11}");

  __asm("CPSIE I");

  __asm("BX LR");
}

uint32_t* choose_next_thread(uint32_t* SP)
{
  if (main_thread_SP_register == 0)
  {
    main_thread_SP_register = SP;
  }
  else if (current_thread != 0)
  {
    current_thread->stack_pointer = SP;

    cyclic_iterator_next(threads_iterator);
  }

  current_thread = (thread_control_block*)iteator_get_data(threads_iterator);

  uint32_t* next_thread_stack_pointer = (current_thread == 0) ? main_thread_SP_register : current_thread->stack_pointer;


  return next_thread_stack_pointer;
}

static void remove_thread_from_kernel_list(void)
{
  __disable_irq();

  //current_thread = iterator_pop(threads_iterator);
  iterator_pop(threads_iterator);
  //thread_control_block_destroy(current_thread);
  current_thread = 0;

  __enable_irq();

  thread_yield();

  // wait forever
  while(1);
}

static void system_timer_initialize(uint32_t CPU_frequency)
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

  register const uint32_t CLKSOURCE = (1U << 2);
  register const uint32_t TICKINT   = (1U << 1);
  register const uint32_t ENABLE    = (1U << 0);

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

static void delay_timer_initialize(uint32_t CPU_frequency)
{

}

static uint32_t get_CPU_frequency()
{
  // TODO: get current CPU clock frequency
  return 4000000;
}
