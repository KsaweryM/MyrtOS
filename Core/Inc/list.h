#ifndef _LIST_H
#define _LIST_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct list_item
{
  struct list_item* next;
  struct list_item* previous;
  void* data;
} list_item;

typedef struct list
{
  list_item* list_begin;
  list_item* list_end;
} list;

typedef struct iterator
{
  list_item* current_list_item;
  list* iterator_owner;
} iterator;

list* list_create(void);

// list must be empty before being destroyed, otherwise it will create memory leak
int32_t list_destroy(list* list_object);

void list_push_front(list* list_object, void* data);
void list_push_back(list* list_object, void* data);

iterator* iterator_create(list* list_object);
void iterator_destroy(iterator* iterator_object);


static inline void* iteator_get_data(const iterator* iterator_object);
static inline void* iterator_pop(iterator* iterator_object);
static inline void iterator_reset(iterator* iterator_object);
static inline uint32_t iterator_next(iterator* iterator_object);
static inline uint32_t iterator_previous(iterator* iterator_object);
static inline uint32_t cyclic_iterator_next(iterator* iterator_object);
static inline uint32_t cyclic_iterator_previous(iterator* iterator_object);

static inline void* iteator_get_data(const iterator* iterator_object)
{
  if (iterator_object->current_list_item == 0)
  {
    return 0;
  }

  return iterator_object->current_list_item->data;
}

static inline void* iterator_pop(iterator* iterator_object)
{
  if (iterator_object->current_list_item == 0)
  {
    return 0;
  }

  list_item* current_list_item = iterator_object->current_list_item;
  list_item* previous_list_item = current_list_item->previous;
  list_item* next_list_item = current_list_item->next;

  if (previous_list_item != 0 && next_list_item != 0)
  {
    previous_list_item->next = next_list_item;
    next_list_item->previous = previous_list_item;
  }
  else if (previous_list_item == 0)
  {
    next_list_item->previous = 0;
    iterator_object->iterator_owner->list_begin = next_list_item;
  }
  else if (next_list_item == 0)
  {
    previous_list_item->next = 0;
    iterator_object->iterator_owner->list_end = previous_list_item;
  }
  else
  {
    iterator_object->iterator_owner->list_begin = 0;
    iterator_object->iterator_owner->list_end = 0;
  }

  void* data = current_list_item->data;

  iterator_object->current_list_item = next_list_item;

  free(current_list_item);

  return data;
}

static inline void iterator_reset(iterator* iterator_object)
{
  iterator_object->current_list_item = iterator_object->iterator_owner->list_begin;
}

static inline uint32_t iterator_next(iterator* iterator_object)
{
  if (iterator_object->current_list_item != 0)
  {
    iterator_object->current_list_item = iterator_object->current_list_item->next;
  }

  return iterator_object->current_list_item == 0;
}


static inline uint32_t iterator_previous(iterator* iterator_object)
{
  if (iterator_object->current_list_item != 0)
  {
    iterator_object->current_list_item = iterator_object->current_list_item->previous;
  }

  return iterator_object->current_list_item == 0;
}

static inline uint32_t cyclic_iterator_next(iterator* iterator_object)
{
  if (iterator_object->current_list_item == 0 || iterator_object->current_list_item->next == 0)
  {
    iterator_object->current_list_item = iterator_object->iterator_owner->list_begin;
  }
  else
  {
    iterator_object->current_list_item = iterator_object->current_list_item->next;
  }

  return iterator_object->current_list_item == 0;
}
static inline uint32_t cyclic_iterator_previous(iterator* iterator_object)
{
  if (iterator_object->current_list_item == 0 || iterator_object->current_list_item->previous == 0)
  {
    iterator_object->current_list_item = iterator_object->iterator_owner->list_end;
  }
  else
  {
    iterator_object->current_list_item = iterator_object->current_list_item->previous;
  }

  return iterator_object->current_list_item == 0;
}

#endif
