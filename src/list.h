#ifndef LIST_H
#define LIST_H

typedef int                 ListComparator(void *, void *);
typedef struct ListNode     ListNode;
typedef struct ListIterator ListIterator;
typedef struct List         List;

struct ListNode {
  ListNode *prev;
  ListNode *next;
  void     *data;
};

struct ListIterator {
  List     *parent;
  ListNode *node;
};

struct List {
  ListNode      head;
  unsigned long size;
};

void list_init(List *list);
void list_deinit(List *list);

void  list_iterator(ListIterator *iterator, List *list);
int   list_iterator_next(ListIterator *iterator);
void *list_iterator_value(ListIterator *iterator);
void  list_iterator_insert(ListIterator *iterator, void *value);
void  list_iterator_update(ListIterator *iterator, void *value);
void  list_iterator_erase(ListIterator *iterator);

unsigned long list_size(List *list);
void         *list_front(List *list);
void         *list_back(List *list);
void          list_push_front(List *list, void *value);
void          list_push_back(List *list, void *value);
void          list_pop_front(List *list);
void          list_pop_back(List *list);
void          list_sort(List *list, ListComparator *comparator);

#endif
