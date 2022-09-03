#ifndef __SCHEDULER_ROUND_ROBIN_H
#define __SCHEDULER_ROUND_ROBIN_H

#include "kernel/scheduler/scheduler.h"

typedef struct scheduler_round_robin_t scheduler_round_robin_t;

scheduler_round_robin_t* scheduler_round_robin_create();

#endif
