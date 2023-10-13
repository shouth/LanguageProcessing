#include <stdlib.h>

#include "list.h"
#include "utility.h"

void list_init(List *list)
{
  list->head.prev = &list->head;
  list->head.next = &list->head;
  list->head.data = NULL;
  list->size      = 0;
}

void list_deinit(List *list)
{
  while (list_size(list) > 0) {
    list_pop_back(list);
  }
}

static void list_insert_node(List *list, ListNode *before, void *value)
{
  ListNode *node   = xmalloc(sizeof(ListNode));
  node->data       = value;
  node->prev       = before;
  node->next       = before->next;
  node->prev->next = node;
  node->next->prev = node;
  ++list->size;
}

static void list_erase_node(List *list, ListNode *node)
{
  node->prev->next = node->next;
  node->next->prev = node->prev;
  free(node);
  --list->size;
}

void list_iterator(ListIterator *iterator, List *list)
{
  iterator->parent = list;
  iterator->node   = &list->head;
}

int list_iterator_next(ListIterator *iterator)
{
  iterator->node = iterator->node->next;
  return iterator->node != &iterator->parent->head;
}

void *list_iterator_value(ListIterator *iterator)
{
  return iterator->node->data;
}

void list_iterator_insert(ListIterator *iterator, void *value)
{
  list_insert_node(iterator->parent, iterator->node, value);
}

void list_iterator_update(ListIterator *iterator, void *value)
{
  iterator->node->data = value;
}

void list_iterator_erase(ListIterator *iterator)
{
  ListNode *node = iterator->node;
  iterator->node = iterator->node->prev;
  list_erase_node(iterator->parent, node);
}

unsigned long list_size(List *list)
{
  return list->size;
}

void *list_front(List *list)
{
  return list->head.next->data;
}

void *list_back(List *list)
{
  return list->head.prev->data;
}

void list_push_front(List *list, void *value)
{
  list_insert_node(list, &list->head, value);
}

void list_push_back(List *list, void *value)
{
  list_insert_node(list, list->head.prev, value);
}

void list_pop_front(List *list)
{
  if (list_size(list) > 0) {
    list_erase_node(list, list->head.next);
  }
}

void list_pop_back(List *list)
{
  if (list_size(list) > 0) {
    list_erase_node(list, list->head.prev);
  }
}

void list_sort(List *list, ListComparator *comparator);
