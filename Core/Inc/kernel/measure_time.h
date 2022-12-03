#ifndef MEASURE_H
#define MEASURE_H

#include <stdint.h>

void measure_start(void);
uint32_t measure_get(void);
uint32_t measure_end(void);

#endif
