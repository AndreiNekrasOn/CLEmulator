#ifndef CLEMULATOR_LIST_H
#define CLEMULATOR_LIST_H
typedef struct list
{
    char* word;
    struct list* next;
} list;

list* list_init();
void list_print(const list* head);
void list_free(list* head);
void list_free_no_words(list* head);
int _list_size_tailrec(list* head, int acc);
int list_size(list* head);
list* list_insert(list* tail, char* data);
#endif
