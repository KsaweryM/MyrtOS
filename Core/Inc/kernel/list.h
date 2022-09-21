#ifndef _LIST_H
#define _LIST_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct list_item_t list_item_t;

typedef struct list_t list_t;

typedef struct iterator_t iterator_t;

list_t* list_create(void);

// list must be empty before being destroyed, otherwise it will create memory leak
int32_t list_destroy(list_t* list);

void list_push_front(list_t* list, void* data);
void list_push_back(list_t* list, void* data);
uint32_t list_is_empty(const list_t* list);

iterator_t* iterator_create(list_t* list);
void iterator_destroy(iterator_t* iterator);

void* iterator_get_data(const iterator_t* iterator);
void* iterator_pop(iterator_t* iterator);
void iterator_reset(iterator_t* iterator);

uint32_t iterator_next(iterator_t* iterator);
uint32_t iterator_previous(iterator_t* iterator);
void iterator_push_previous(iterator_t* iterator, void* data);

void* cyclic_iterator_pop(iterator_t* iterator);
uint32_t cyclic_iterator_next(iterator_t* iterator);
uint32_t cyclic_iterator_previous(iterator_t* iterator);

void iterator_remove_item_by_data(iterator_t* iterator, void* data);

#endif
