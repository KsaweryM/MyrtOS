#ifndef _LIST_H
#define _LIST_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct list_item list_item;

typedef struct list list;

typedef struct iterator iterator;

list* list_create(void);

// list must be empty before being destroyed, otherwise it will create memory leak
int32_t list_destroy(list* list_object);

void list_push_front(list* list_object, void* data);
void list_push_back(list* list_object, void* data);
uint32_t list_is_empty(const list* list_object);

iterator* iterator_create(list* list_object);
void iterator_destroy(iterator* iterator_object);

void* iterator_get_data(const iterator* iterator_object);
void* iterator_pop(iterator* iterator_object);
void iterator_reset(iterator* iterator_object);

uint32_t iterator_next(iterator* iterator_object);
uint32_t iterator_previous(iterator* iterator_object);
uint32_t cyclic_iterator_next(iterator* iterator_object);
uint32_t cyclic_iterator_previous(iterator* iterator_object);

#endif
