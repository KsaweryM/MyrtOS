#include <kernel/list.h>

struct list_item
{
  struct list_item* next;
  struct list_item* previous;
  void* data;
};

struct list
{
  list_item* list_begin;
  list_item* list_end;
};

struct iterator
{
  list_item* current_list_item;
  list* iterator_owner;
};

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

uint32_t list_is_empty(const list* list_object)
{
	return list_object->list_begin == 0;
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

void* iterator_get_data(const iterator* iterator_object)
{
  if (iterator_object->current_list_item == 0)
  {
    return 0;
  }

  return iterator_object->current_list_item->data;
}

void* iterator_pop(iterator* iterator_object)
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

void iterator_reset(iterator* iterator_object)
{
  iterator_object->current_list_item = iterator_object->iterator_owner->list_begin;
}

uint32_t iterator_next(iterator* iterator_object)
{
  if (iterator_object->current_list_item != 0)
  {
    iterator_object->current_list_item = iterator_object->current_list_item->next;
  }

  return iterator_object->current_list_item == 0;
}


uint32_t iterator_previous(iterator* iterator_object)
{
  if (iterator_object->current_list_item != 0)
  {
    iterator_object->current_list_item = iterator_object->current_list_item->previous;
  }

  return iterator_object->current_list_item == 0;
}

void* cyclic_iterator_pop(iterator* iterator_object)
{
	void* data = iterator_pop(iterator_object);

	if (iterator_object->current_list_item == 0)
	{
		 iterator_object->current_list_item = iterator_object->iterator_owner->list_begin;
	}

	return data;
}

uint32_t cyclic_iterator_next(iterator* iterator_object)
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

uint32_t cyclic_iterator_previous(iterator* iterator_object)
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
