#include <kernel/list.h>

struct list_item_t
{
  struct list_item_t* next;
  struct list_item_t* previous;
  void* data;
};

struct list_t
{
  list_item_t* list_begin;
  list_item_t* list_end;
};

struct iterator_t
{
  list_item_t* current_list_item;
  list_t* iterator_owner;
};

list_t* list_create(void)
{
  list_t* list = malloc(sizeof(*list));

  list->list_begin = 0;
  list->list_end = 0;

  return list;
}

int32_t list_destroy(list_t* list)
{
  if (list->list_begin != 0)
  {
    return -1;
  }

  free(list);

  return 0;
}


void list_push_front(list_t* list, void* data)
{
  list_item_t* list_item = malloc(sizeof(*list_item));

  list_item->previous = 0;
  list_item->next = 0;
  list_item->data = data;


  if (list->list_begin == 0)
  {
    list->list_begin = list_item;
    list->list_end = list_item;
  }
  else
  {
    list_item->next = list->list_begin;
    list->list_begin->previous = list_item;

    list->list_begin = list_item;
  }
}

void list_push_back(list_t* list, void* data)
{
  list_item_t* list_item = malloc(sizeof(*list_item));

  list_item->previous = 0;
  list_item->next = 0;
  list_item->data = data;

  if (list->list_begin == 0)
  {
    list->list_begin = list_item;
    list->list_end = list_item;
  }
  else
  {
    list_item->previous = list->list_end;
    list->list_end->next = list_item;

    list->list_end = list_item;
  }
}

uint32_t list_is_empty(const list_t* list)
{
	return list->list_begin == 0;
}

iterator_t* iterator_create(list_t* list)
{
  iterator_t* iterator = malloc(sizeof(*iterator));

  iterator->current_list_item = list->list_begin;
  iterator->iterator_owner = list;

  return iterator;
}

void iterator_destroy(iterator_t* iterator)
{
  free(iterator);
}

void* iterator_get_data(const iterator_t* iterator)
{
  if (iterator->current_list_item == 0)
  {
    return 0;
  }

  return iterator->current_list_item->data;
}

void* iterator_pop(iterator_t* iterator)
{
  if (iterator->current_list_item == 0)
  {
    return 0;
  }

  list_item_t* current_list_item = iterator->current_list_item;
  list_item_t* previous_list_item = current_list_item->previous;
  list_item_t* next_list_item = current_list_item->next;

  if (previous_list_item != 0 && next_list_item != 0)
  {
    previous_list_item->next = next_list_item;
    next_list_item->previous = previous_list_item;
  }
  else if (previous_list_item == 0)
  {
    next_list_item->previous = 0;
    iterator->iterator_owner->list_begin = next_list_item;
  }
  else if (next_list_item == 0)
  {
    previous_list_item->next = 0;
    iterator->iterator_owner->list_end = previous_list_item;
  }
  else
  {
    iterator->iterator_owner->list_begin = 0;
    iterator->iterator_owner->list_end = 0;
  }

  void* data = current_list_item->data;

  iterator->current_list_item = next_list_item;

  free(current_list_item);

  return data;
}

void iterator_reset(iterator_t* iterator)
{
  iterator->current_list_item = iterator->iterator_owner->list_begin;
}

uint32_t iterator_next(iterator_t* iterator)
{
  if (iterator->current_list_item != 0)
  {
    iterator->current_list_item = iterator->current_list_item->next;
  }

  return iterator->current_list_item == 0;
}


uint32_t iterator_previous(iterator_t* iterator)
{
  if (iterator->current_list_item != 0)
  {
    iterator->current_list_item = iterator->current_list_item->previous;
  }

  return iterator->current_list_item == 0;
}

void* cyclic_iterator_pop(iterator_t* iterator)
{
	void* data = iterator_pop(iterator);

	if (iterator->current_list_item == 0)
	{
		 iterator->current_list_item = iterator->iterator_owner->list_begin;
	}

	return data;
}

uint32_t cyclic_iterator_next(iterator_t* iterator)
{
  if (iterator->current_list_item == 0 || iterator->current_list_item->next == 0)
  {
    iterator->current_list_item = iterator->iterator_owner->list_begin;
  }
  else
  {
    iterator->current_list_item = iterator->current_list_item->next;
  }

  return iterator->current_list_item == 0;
}

uint32_t cyclic_iterator_previous(iterator_t* iterator)
{
  if (iterator->current_list_item == 0 || iterator->current_list_item->previous == 0)
  {
    iterator->current_list_item = iterator->iterator_owner->list_end;
  }
  else
  {
    iterator->current_list_item = iterator->current_list_item->previous;
  }

  return iterator->current_list_item == 0;
}
