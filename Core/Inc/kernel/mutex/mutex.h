#ifndef __MUTEX_H
#define __MUTEX_H

typedef struct mutex_t mutex_t;

mutex_t* mutex_create(void);
void mutex_lock(mutex_t* mutex);
void mutex_unlock(mutex_t* mutex);
void mutex_destroy(mutex_t* mutex);

#endif
