#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
void
lst_init(struct list_node *lst)
{
  lst->next = lst;
  lst->prev = lst;
}

int
lst_empty(struct list_node *lst) {
  return lst->next == lst;
}

void
lst_remove(struct list_node *e) {
  e->prev->next = e->next;
  e->next->prev = e->prev;
}

void*
lst_pop(struct list_node *lst) {
  if(lst->next == lst)
    panic("lst_pop");
  struct list_node *p = lst->next;
  lst_remove(p);
  return (void *)p;
}

void
lst_push(struct list_node *lst, void *p)
{
  struct list_node *e = (struct list_node *) p;
  e->next = lst->next;
  e->prev = lst;
  lst->next->prev = p;
  lst->next = e;
}

// void lst_print(struct list_node *lst) {
//     for (struct list_node *p = lst->next; p != lst; p = p->next) {
//         printf(" %p", p->data);
//     }
//     printf("\n");
// }

