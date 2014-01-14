#include "cp-list.h"

void __list_add(struct list_head *new, 
        struct list_head *prev,
        struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

void list_add(struct list_head *new, struct list_head *head)
{
    __list_add(new, head, head->next);
}

static inline void __list_del(struct list_head * prev, struct list_head * next)
{    
    next->prev = prev;
    prev->next = next;
}

void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = LIST_POISON1;
    entry->prev = LIST_POISON2;
}
