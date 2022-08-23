#include "list.h"

list* list_create(void)
{
  list* list_object = malloc(sizeof(*list_object));

  list_object->list_begin = 0;
  list_object->list_end = 0;

  return list_object;
}

int32_t list_destroy(list* list_object)
{
  if (list_object->list_begin != 0)
  {
    return -1;
  }

  free(list_object);

  return 0;
}


void list_push_front(list* list_object, void* data)
{
  list_item* list_item_object = malloc(sizeof(*list_item_object));

  list_item_object->previous = 0;
  list_item_object->next = 0;
  list_item_object->data = data;


  if (list_object->list_begin == 0)
  {
    list_object->list_begin = list_item_object;
    list_object->list_end = list_item_object;
  }
  else
  {
    list_item_object->next = list_object->list_begin;
    list_object->list_begin->previous = list_item_object;

    list_object->list_begin = list_item_object;
  }
}

void list_push_back(list* list_object, void* data)
{
  list_item* list_item_object = malloc(sizeof(*list_item_object));

  list_item_object->previous = 0;
  list_item_object->next = 0;
  list_item_object->data = data;

  if (list_object->list_begin == 0)
  {
    list_object->list_begin = list_item_object;
    list_object->list_end = list_item_object;
  }
  else
  {
    list_item_object->previous = list_object->list_end;
    list_object->list_end->next = list_item_object;

    list_object->list_end = list_item_object;
  }
}

iterator* iterator_create(list* list_object)
{
  iterator* iterator_object = malloc(sizeof(*iterator_object));

  iterator_object->current_list_item = list_object->list_begin;
  iterator_object->iterator_owner = list_object;

  return iterator_object;
}

void iterator_destroy(iterator* iterator_object)
{
  free(iterator_object);
}
