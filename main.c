#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

enum
{
    default_string_cap = 32
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
    list* child;
    if (tail == NULL)
        return list_init();
    child = list_init();
    tail->next = child;
    return child;
}

char* scan_command()
{
    int symbol;
    int size = 0;
    int capacity = default_string_cap;
    char* command_str = NULL;
    while ((symbol = getchar()) != EOF)
    {
        if (symbol == '\n')
            return command_str;
        command_str = update_str(command_str, symbol, &size, &capacity);
    }
    return command_str;
}

int start_new_word(int new_word_flag, list** tail)
{
    if (new_word_flag)
    {
        *tail = list_insert(*tail);
        new_word_flag = 0;
    }
    return new_word_flag;
}

void mutate_to_default(int* word_size, int* word_cap, int* quote_flag,
                       int* new_word_flag)
{
    *word_size = 0;
    *word_cap = default_string_cap;
    *quote_flag = 0;
    *new_word_flag = 1;
}

list* parse_command(const char* command)
{
    list* head = NULL;
    list* tail = NULL;
    int i;
    int word_size, word_cap, quote_flag, new_word_flag;
    if (command == NULL)
        return NULL;
    mutate_to_default(&word_size, &word_cap, &quote_flag, &new_word_flag);
    for (i = 0; command[i] != '\0'; i++)
    {
        if (command[i] == ' ' && !quote_flag)
        {
            if (word_size == 0)
                continue;
            mutate_to_default(&word_size, &word_cap, &quote_flag,
                              &new_word_flag);
        }
        else
        {
            if (command[i] == '"')
                quote_flag = !quote_flag;
            new_word_flag = start_new_word(new_word_flag, &tail);
            if (head == NULL)
                head = tail;
            tail->word
                = update_str(tail->word, command[i], &word_size, &word_cap);
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

char** list_to_argv(list** head)
{
    char** arr;
    int size;
    list* curr;
    int i;
    if (head == NULL)
        return NULL;
    size = list_size(*head) + 1; /* all words from list and NULL at the end */
    arr = malloc(size * sizeof(*arr));
    for (curr = *head, i = 0; curr != NULL && i < size; curr = curr->next, i++)
    {
        arr[i] = curr->word;
    }
    arr[size - 1] = NULL;
    list_free_no_words(*head);
    return arr;
}

void free_argv(char** argv)
{
    int i;
    for (i = 0; argv != NULL && argv[i] != NULL; i++)
    {
        free(argv[i]);
    }
    free(argv);
}

int check_cd_command(char* name)
{
    return !strcmp(name, "cd");
}

void perform_cd_command(char* dir)
{
    int err_code;
    err_code = chdir(dir);
    if (err_code)
        perror(dir);
}

void perform_command(char** argv)
{
    if (check_cd_command(argv[0]))
    {
        perform_cd_command(argv[1]);
    }
    else
    {
        if (!fork())
        {
            execvp(argv[0], argv);
            perror(argv[0]);
            exit(1);
        }
        wait(NULL);
    }
}

int main()
{
    list* command;
    char** cmd_argv;
    while (!feof(stdin))
    {
        printf(">>");
        command = parse_command(scan_command());
        if (command == NULL)
            continue;
        list_print(command);
        cmd_argv = list_to_argv(&command);
        perform_command(cmd_argv);
        free_argv(cmd_argv);
    }
    return 0;
}
