#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
    DEFAULT_STRING_CAP = 32
};

typedef struct list
{
    char* word;
    struct list* next;
} list;

char* reallocate_str(char* str, int size, int* capacity)
{
    if (size + 1 == *capacity)
    {
        *capacity *= 2;
        str = realloc(str, *capacity);
    }
    return str;
}

list* list_init()
{
    list* l = malloc(sizeof(*l));
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
    if (head == NULL)
        return;
    list* next = head->next;
    free(head->word);
    free(head);
    list_free(next);
}

char* update_str(char* str, char symbol, int* size, int* capacity)
{
    if (str == NULL) /* word is not initiated */
    {
        str = malloc(*capacity);
        *size = 0;
    }
    str = reallocate_str(str, *size, capacity);
    str[(*size)++] = symbol;
    str[(*size)] = '\0';
    return str;
}

list* list_insert(list* tail)
{
    if (tail == NULL)
        return list_init();
    list* child = list_init();
    tail->next = child;
    return child;
}

char* scan_command()
{
    int symbol;
    int size = 0;
    int capacity = DEFAULT_STRING_CAP;
    char* command_str = NULL;
    while ((symbol = getchar()) != EOF)
    {
        if (symbol == '\n')
            return command_str;
        command_str = update_str(command_str, symbol, &size, &capacity);
    }
    return command_str;
}

int start_new_word(int new_word_flag, list* tail)
{
    if (new_word_flag)
    {
        tail = list_insert(tail);
        new_word_flag = 0;
    }
    return new_word_flag;
}

void mutate_to_default(int* word_size, int* word_cap, int* quote_flag, int* new_word_flag)
{
    *word_size = 0;
    *word_cap = DEFAULT_STRING_CAP;
    *quote_flag = 0;
    *new_word_flag = 1;
}

list* parse_command(const char* command)
{
    if (command == NULL)
        return NULL;
    list* head = NULL;
    list* tail = NULL;
    int word_size, word_cap, quote_flag, new_word_flag;
    mutate_to_default(&word_size, &word_cap, &quote_flag, &new_word_flag);
    int i;
    for (i = 0; command[i] != '\0'; i++)
    {
        if (command[i] == ' ' && !quote_flag)
        {
            if (word_size == 0)
                continue;
            mutate_to_default(&word_size, &word_cap, &quote_flag, &new_word_flag);
        }
        else
        {
            if (command[i] == '"')
                quote_flag = !quote_flag;
            new_word_flag = start_new_word(new_word_flag, tail);
            if (head == NULL)
                head = tail;
            tail->word = update_str(tail->word, command[i], &word_size, &word_cap);
        }
    }
    if (quote_flag)
    {
        fprintf(stderr, "Error - unbalanced quotes\n");
        list_free(head);
        return NULL;
    }
    return head;
}

int main()
{
    while (!feof(stdin))
    {
        printf(">>");
        list* command = parse_command(scan_command());
        list_print(command);
        list_free(command);
    }
    return 0;
}
