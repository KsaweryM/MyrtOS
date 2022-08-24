#ifndef __ATOMIC_H
#define __ATOMIC_H

#include <stdint.h>
#include "stm32l4xx.h"

extern volatile int32_t critical_path_depth;

#define CRITICAL_PATH_ENTER()       \
  do                                \
  {                                 \
  __disable_irq();                  \
    critical_path_depth++;          \
  } while (0);

#define CRITICAL_PATH_EXIT()       \
  do                               \
  {                                \
    critical_path_depth--;         \
    if (critical_path_depth == 0)  \
    {                              \
      __enable_irq();              \
    }                              \
  } while (0);


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
