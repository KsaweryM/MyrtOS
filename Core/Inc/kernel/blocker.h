#ifndef __BLOCKER_H
#define __BLOCKER_H

#include <kernel/thread.h>

typedef struct blocker_t blocker_t;

blocker_t* blocker_create(scheduler_t* scheduler);
void blocker_destroy(blocker_t* blocker);
void blocker_block_thread(blocker_t* blocker, thread_control_block_t* thread);

#endif
