#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "list.h"

enum
{
    default_string_cap = 32
};

enum separator_type
{
    not_separator,
    space,
    daemon,
    redirect_in,
    redirect_out,
    append_in
};

char* reallocate_str(char* str, int size, int* capacity)
{
    if (size + 1 == *capacity)
    {
        *capacity *= 2;
        str = realloc(str, *capacity);
    }
    return str;
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

void update_word(int* new_word_flag, list** tail, list** head,
                 const char* symbols, int len, int* word_size,
                 int* word_cap)
{
    int i;
    if (*new_word_flag)
    {
        *tail = list_insert(*tail);
        *new_word_flag = 0;
    }
    if (*head == NULL)
        *head = *tail;
    for (i = 0; i < len; i++)
        (*tail)->word
            = update_str((*tail)->word, symbols[i], word_size, word_cap);
}

void mutate_to_default(int* word_size, int* word_cap, int* quote_flag,
                       int* new_word_flag)
{
    *word_size = 0;
    *word_cap = default_string_cap;
    *quote_flag = 0;
    *new_word_flag = 1;
}

enum separator_type identify_separator(char* separator)
{
    if (separator == NULL)
        return not_separator;
    if (!strncmp(separator, ">>", 2))
        return append_in;
    if (!strncmp(separator, ">", 1))
        return redirect_in;
    if (!strncmp(separator, "<", 1))
        return redirect_out;
    if (!strncmp(separator, "&", 1))
        return daemon;
    if (isspace(separator[0]))
        return space;
    return not_separator;
}

enum separator_type check_separator_list(const char* str_i,
                                         const list* separators, int quote_flag)
{
    list sep_copy;
    if (str_i == NULL || quote_flag)
        return not_separator;
    if (separators == NULL) /* only tokenize spaces */
        return isspace(*str_i) ? space : not_separator;
    for (sep_copy = *separators; sep_copy.next != NULL; sep_copy = *sep_copy.next)
    {
        if (!strncmp(str_i, sep_copy.word, strlen(sep_copy.word)))
            return identify_separator(sep_copy.word);
    }
    return not_separator;
}

char* get_separator_value_by_type(enum separator_type st)
{
    switch (st)
    {
    case daemon:
        return "&";
    case append_in:
        return ">>";
    case redirect_in:
        return ">";
    case redirect_out:
        return "<";
    default:
        fprintf(stderr, "Illegal argument for get_separator_by_type");
        return NULL;
    }
}

list* tokenize_string(const char* str, list* separators)
{
    list *head = NULL, *tail = NULL;
    int i, word_size, word_cap, quote_flag, new_word_flag;
    enum separator_type sep;
    char* sep_value;
    if (str == NULL)
        return NULL;
    mutate_to_default(&word_size, &word_cap, &quote_flag, &new_word_flag);
    for (i = 0; str[i] != '\0'; i++)
    {
        if ((sep = check_separator_list(str + i, separators, quote_flag))
            != not_separator)
        {
            if (sep == space && word_size == 0)
                continue;
            else if (sep != space)
            {
                sep_value = get_separator_value_by_type(sep);
                word_size = strlen(sep_value);
                new_word_flag = 1;
                update_word(&new_word_flag, &tail, &head, sep_value,
                            word_size, &word_size, &word_cap);
                i += (word_size - 1); /* in case of a long token */
            }
            mutate_to_default(&word_size, &word_cap, &quote_flag,
                              &new_word_flag);
        }
        else
        {
            if (str[i] == '"')
                quote_flag = !quote_flag;
            update_word(&new_word_flag, &tail, &head, &str[i], 1,
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
    while (argv[i++] != NULL)
        ;
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
    return argv[argc - 1][0] == '&';
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
            while (wait(NULL) != pid)
                ;
    }
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ; /* remove zombies */
}

list* create_default_separators()
{
    return tokenize_string(">> & < ", NULL);
}

int main()
{
    list* command;
    char** cmd_argv;
    list* separators = create_default_separators();
    char* user_input;
    while (!feof(stdin))
    {
        printf("::$ ");
        user_input = scan_command();
        command = tokenize_string(user_input, separators);
        free(user_input);
        if (command == NULL)
            continue;
        list_print(command);
        cmd_argv = list_to_argv(&command);
        perform_command(cmd_argv);
        free_argv(cmd_argv);
    }
    list_free_no_words(separators);
    puts("\n-----");
    return 0;
}
