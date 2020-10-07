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

enum Separators
{
    daemon_separator = '&'
};

enum ParseStatus
{
    invalid_daemon_format,
    ok
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

void update_word(int* new_word_flag, list** tail, list** head, char symbol,
                 int* word_size, int* word_cap)
{
    if (*new_word_flag)
    {
        *tail = list_insert(*tail);
        *new_word_flag = 0;
    }
    if (*head == NULL)
        *head = *tail;

    (*tail)->word = update_str((*tail)->word, symbol, word_size, word_cap);
}

void mutate_to_default(int* word_size, int* word_cap, int* quote_flag,
                       int* new_word_flag)
{
    *word_size = 0;
    *word_cap = default_string_cap;
    *quote_flag = 0;
    *new_word_flag = 1;
}

int check_separator_list(char symbol, const char* separators,
                         int quote_flag)
{
    if (separators == NULL || quote_flag)
        return 0;
    return strchr(separators, symbol) != NULL;
}

list* parse_command(const char* command, const char* separators)
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
        if (check_separator_list(command[i], separators, quote_flag))
        {
            if (command[i] == ' ' && word_size == 0)
                continue;
            else if (command[i] != ' ')
            {
                word_size = 1;
                new_word_flag = 1;
                update_word(&new_word_flag, &tail, &head, command[i],
                            &word_size, &word_cap);
            }
            mutate_to_default(&word_size, &word_cap, &quote_flag,
                              &new_word_flag);
        }
        else
        {
            if (command[i] == '"')
                quote_flag = !quote_flag;
            update_word(&new_word_flag, &tail, &head, command[i],
                        &word_size, &word_cap);
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
    size = list_size(*head) + 1;
    arr = malloc(size * sizeof(*arr));
    for (curr = *head, i = 0; curr != NULL && i < size;
         curr = curr->next, i++)
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

int get_argc(char** argv)
{
    int i = 0;
    if (argv == NULL || *argv == NULL)
        return 0;
    while (argv[i++] != NULL) {}
    return i - 1;
}

int check_cd_command(char* name)
{
    return !strcmp(name, "cd");
}

int check_daemon(char** argv)
{
    int argc;
    argc = get_argc(argv);
    if (argc < 2)
        return 0;
    return argv[argc - 1][0] == daemon_separator;
}

enum ParseStatus validate_argv(char** argv)
{
    int i;
    if (check_daemon(argv))
    {
        for (i = 0; argv[i] != 0; i++)
        {
            if (argv[i][0] != '"' && (strchr(argv[i], daemon_separator) + 1) != NULL)
                return invalid_daemon_format;
        }
    }
    return ok;
}

void perform_cd_command(const char* dir)
{
    int err_code;
    if (dir == NULL)
    {
        fprintf(stderr, "Argument expected\n");
        return;
    }
    err_code = chdir(dir);
    if (err_code)
        perror(dir);
}

void perform_command(char** argv)
{
    int pid;
    int is_daemon;
    if (check_cd_command(argv[0]))
        perform_cd_command(argv[1]);
    else
    {
        is_daemon = check_daemon(argv);
        pid = fork();
        if (!pid)
        {
            if (is_daemon)
                argv[get_argc(argv) - 1] = NULL;
            execvp(argv[0], argv);
            perror(argv[0]);
            exit(1);
        }
        if (!is_daemon)
            while (wait(NULL) != pid);
    }
    while (waitpid(-1, NULL, WNOHANG) > 0); /* remove zombies */
}

int main()
{
    list* command;
    char** cmd_argv;
    const char* separators = " &";
    char* user_input;
    while (!feof(stdin))
    {
        printf(">>");
        user_input = scan_command();
        command = parse_command(user_input, separators);
        free(user_input);
        if (command == NULL)
            continue;
        list_print(command);
        cmd_argv = list_to_argv(&command);
        printf("program is daemon: %s\n", check_daemon(cmd_argv) ? "true" : "false"); 
        perform_command(cmd_argv);
        free_argv(cmd_argv);
    }
    return 0;
}
