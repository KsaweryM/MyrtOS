#ifndef __ATOMIC_H
#define __ATOMIC_H

#include <stdint.h>
#include <assert.h>
#include "stm32l4xx.h"

extern volatile int32_t critical_path_depth;

#define INTCTRL			(*((volatile uint32_t *)0xE000ED04))
#define PENDSTET		(1U << 26)

void critical_path_depth_test(void);

#define CRITICAL_PATH_ENTER()       \
  do                                \
  {                                 \
  __disable_irq();                  \
    critical_path_depth++;          \
  } while (0);

#define CRITICAL_PATH_EXIT()        \
  do                                \
  {                                 \
    critical_path_depth--;          \
    if (critical_path_depth == 0)   \
    {                               \
      __enable_irq();               \
    }                               \
  } while (0);


void yield();

void yield_after_critical_path();

#define ATOMIC_SET(VARIABLE, VALUE) \
  do                                \
  {                                 \
    __disable_irq();                \
    VARIABLE = VALUE;               \
    __enable_irq();                 \
  } while (0)

#define ATOMIC_INCREMENT(VARIABLE)  \
  do                                \
  {                                 \
    __disable_irq();                \
    VARIABLE++;                     \
    __enable_irq();                 \
  } while (0)

#define ATOMIC_DECREMENT(VARIABLE)  \
  do                                \
  {                                 \
    __disable_irq();                \
    VARIABLE++;                     \
    __enable_irq();                 \
  } while (0)

#endif
