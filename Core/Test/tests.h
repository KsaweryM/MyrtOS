#ifndef __TESTS_H_
#define __TESTS_H_

#define GLOBAL_TEST_REPETITIONS 5

#include <stdint.h>
#include <kernel/scheduler/scheduler.h>

uint32_t test0(SCHEDULER_ALGORITHM scheduler_algorithm);
uint32_t test1(SCHEDULER_ALGORITHM scheduler_algorithm);
uint32_t test2(SCHEDULER_ALGORITHM scheduler_algorithm);
uint32_t test3(SCHEDULER_ALGORITHM scheduler_algorithm);
uint32_t test4(SCHEDULER_ALGORITHM scheduler_algorithm);

#endif
