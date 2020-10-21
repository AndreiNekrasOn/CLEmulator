#include "list.h"
#include <stdio.h>
#include <stdlib.h>

list* list_init()
{
    list* l;
    l = malloc(sizeof(*l));
    l->next = NULL;
    l->word = NULL;
    return l;
}

void list_print(const list* head)
{
    if (head == NULL)
        return;
    printf("%s\n", head->word);
    list_print(head->next);
}

void list_free(list* head)
{
    list* next;
    if (head == NULL)
        return;
    next = head->next;
    free(head->word);
    free(head);
    list_free(next);
}

void list_free_no_words(list* head)
{
    list* next;
    if (head == NULL)
        return;
    next = head->next;
    free(head);
    list_free_no_words(next);
}

int _list_size_tailrec(list* head, int acc)
{
    if (head == NULL)
        return acc;
    return _list_size_tailrec(head->next, acc + 1);
}

int list_size(list* head)
{
    return _list_size_tailrec(head, 0);
}

list* list_insert(list* tail)
{
    list* child;
    child = list_init();
    if (tail != NULL)
        tail->next = child;
    return child;
}
