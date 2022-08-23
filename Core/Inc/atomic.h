#ifndef __ATOMIC_H
#define __ATOMIC_H

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
