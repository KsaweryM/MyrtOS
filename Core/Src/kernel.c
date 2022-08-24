#include "kernel.h"
#include "scheduler.h"
#include "atomic.h"
#include "list.h"

#define INTCTRL (*((volatile uint32_t *)0xE000ED04))
#define PENDSTET  (1U<<26)

static void system_timer_initialize(uint32_t CPU_frequency);
static void delay_timer_initialize(uint32_t CPU_frequency);
static uint32_t get_CPU_frequency();

scheduler* scheduler_object_pointer = 0;

void kernel_create(void)
{
  scheduler_object_pointer = scheduler_create(0);
}

void kernel_destroy(void)
{

}

void kernel_launch(void)
{
  uint32_t CPU_frequency = get_CPU_frequency();

  system_timer_initialize(CPU_frequency);
  delay_timer_initialize(CPU_frequency);

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
  scheduler_add_thread(scheduler_object_pointer, thread_attributes_object);
}

void thread_delay(uint32_t seconds)
{
}

void thread_yield(void)
{
  __disable_irq();
  SysTick->VAL = 0;

  INTCTRL = PENDSTET;
  __enable_irq();
}

uint32_t kernel_is_context_to_save()
{
  return scheduler_is_context_to_save(scheduler_object_pointer);
}

uint32_t kernel_choose_next_thread(uint32_t SP_register)
{
  return scheduler_choose_next_thread(scheduler_object_pointer, SP_register);
}


__attribute__((naked)) void SysTick_Handler(void)
{
  __asm("CPSID  I");

  __asm("PUSH {LR}");

  __asm("BL kernel_is_context_to_save");

  __asm("POP {LR}");

  __asm("CMP R0, #0");

  __asm("BEQ save_contex_end");

  __asm("save_contex_begin: PUSH {R4-R11}");

  __asm("save_contex_end: MOV R0, SP");

  __asm("PUSH {LR}");

  __asm("BL kernel_choose_next_thread");

  __asm("POP {LR}");

  __asm("MOV SP, R0");

  __asm("POP {R4-R11}");

  __asm("CPSIE I");

  __asm("BX LR");
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
