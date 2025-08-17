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

static int int_compare(ListNode const *left, ListNode const *right)
{
  Int const *l = container_of(left, Int, node);
  Int const *r = container_of(right, Int, node);
  return l->value - r->value;
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
    ListNode *n = list.next;
    for (; n != list.prev; n = n->next) {
      Int const *current = container_of(n, Int, node);
      Int const *next = container_of(n->next, Int, node);
      assert(current->value <= next->value);
    }
  }

  return 0;
}
