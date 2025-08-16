#include "util.h"

typedef struct Int Int;

struct Int {
  ListNode node;
  int value;
};

static void push(ListNode *head, int value)
{
  Int *item = xmalloc(sizeof(*item));
  item->value = value;
  list_push_back(head, &item->node);
}

static int int_compare(ListNode const *a, ListNode const *b)
{
  Int const *left = container_of(a, Int, node);
  Int const *right = container_of(b, Int, node);
  return left->value - right->value;
}

int main(void)
{
  ListNode list;
  list_init(&list);

  push(&list, 3);
  push(&list, 1);
  push(&list, 4);
  push(&list, 1);
  push(&list, 4);
  push(&list, 5);

  list_sort(&list, int_compare);

  {
    ListNode *ptr = list.next;
    for (; ptr != list.prev; ptr = ptr->next) {
      Int const *current = container_of(ptr, Int, node);
      Int const *next = container_of(ptr->next, Int, node);
      assert(current->value <= next->value);
    }
  }
}
