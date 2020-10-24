#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "argv_processing.h"
#include "list.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

enum
{
    default_string_cap = 32
};

enum separator_type
{
    not_separator,
    space,
    daemon,
    redirect_stdout,
    redirect_stdin,
    redirect_stdout_a,
    pipe_line
};

/* action on signal SIGCHLD */
void remove_zombies(int n)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

/* str memory and input */
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

/* parser */

/* separator processing */
enum separator_type identify_separator(char* separator)
{
    if (separator == NULL)
        return not_separator;
    if (!strcmp(separator, ">>"))
        return redirect_stdout_a;
    if (!strcmp(separator, ">"))
        return redirect_stdout;
    if (!strcmp(separator, "<"))
        return redirect_stdin;
    if (!strcmp(separator, "&"))
        return daemon;
    if (!strcmp(separator, "|"))
        return pipe_line;
    if (isspace(separator[0]))
        return space;
    return not_separator;
}

enum separator_type check_separator_list(const char* str_i,
                                         const list* separators,
                                         int quote_flag)
{
    list sep_copy;
    if (str_i == NULL || quote_flag)
        return not_separator;
    if (separators == NULL || isspace(*str_i)) /* only tokenize spaces */
        return isspace(*str_i) ? space : not_separator;
    for (sep_copy = *separators; sep_copy.next != NULL;
         sep_copy = *sep_copy.next)
    {
        if (!strncmp(str_i, sep_copy.word, strlen(sep_copy.word)))
            return identify_separator(sep_copy.word);
    }
    if (!strncmp(str_i, sep_copy.word, strlen(sep_copy.word)))
        return identify_separator(sep_copy.word);
    return not_separator;
}

char* get_separator_value_by_type(enum separator_type st)
{
    switch (st)
    {
    case daemon:
        return "&";
    case redirect_stdout_a:
        return ">>";
    case redirect_stdout:
        return ">";
    case redirect_stdin:
        return "<";
    case pipe_line:
        return "|";
    default:
        return NULL;
    }
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

/* performing programm */
int is_daemon(char** argv)
{
    int argc;
    argc = get_argc(argv);
    return argc > 1 && argv_contains(argv, "&") == argc - 1;
}

int perform_redirect(char* filename, int strem_fd, int flags)
{
    int fd;
    if (filename == NULL)
    {
        fprintf(stderr, "Filename expected\n");
        return -1;
    }
    fd = open(filename, flags, 0666);
    if (fd == -1)
        return -1;
    dup2(fd, strem_fd);
    close(fd);
    return 0;
}

int check_and_perform_redirect(char* argv[])
{
    int token_idx;
    int success = 1;
    if ((token_idx = retrieve_from_argv(argv, ">")) != -1)
        success &= !perform_redirect(
            argv[token_idx], 1, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY);
    else if ((token_idx = retrieve_from_argv(argv, ">>")) != -1)
        success &= !perform_redirect(argv[token_idx], 1,
                                     O_WRONLY | O_APPEND | O_BINARY);
    if (token_idx != -1)
        remove_from_argv(argv, token_idx); /* remove filename */
    if ((token_idx = retrieve_from_argv(argv, "<")) != -1)
    {
        success
            &= !perform_redirect(argv[token_idx], 0, O_RDONLY | O_BINARY);
        remove_from_argv(argv, token_idx);
    }
    return success ? 0 : -1;
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

void perform_single_command(char** argv, int daemon_flag)
{
    int pid;
    if (argv_contains(argv, "cd") == 0)
        perform_cd_command(argv[1]);
    else
    {
        pid = fork();
        if (!pid)
        {
            if (daemon_flag)
                argv[get_argc(argv) - 1] = NULL;
            if (check_and_perform_redirect(argv) == -1)
            {
                fprintf(stderr, "Cannot perform redirection\n");
                exit(1);
            }
            execvp(argv[0], argv);
            perror(argv[0]);
            exit(1);
        }
        if (!daemon_flag)
            while (wait(NULL) != pid)
                ;
    }
}

void perform_pipe(char*** piped, int daemon_flag)
{
    perform_single_command(piped[0], daemon_flag);
}

void perform_command(char* argv[])
{

    char*** piped;
    int num_pipes, daemon_flag;
    if (is_argv_valid(argv))
    {
        daemon_flag = is_daemon(argv);
        /* retrieve_from_argv(argv, "&");
           // need to find a way to separate & and > >> < commands from
           others
           // maybe something like argv_cut(char* argv[], int position_to)
           - everything after position two is retrieved...
           // but only after figuring input/output streams/files
           (name==NULL => standard)
           // and after figuring daemon / non daemon
        */
        num_pipes = count_pipes(argv);
        piped = pipe_split_argv(argv);
        print_piped_argv(piped, num_pipes);
        if (num_pipes == 1)
            perform_single_command(piped[0], daemon_flag);
        else if (is_piped_valid(piped, num_pipes))
            perform_pipe(piped, daemon_flag);
        free_piped_argv(piped, num_pipes);
    }
    else
        free_argv(argv);
}

/* main method */
int main(int argc, char* argv[])
{
    list* command;
    char* user_input;
    list* separators;
    signal(SIGCHLD, remove_zombies);
    separators = tokenize_string(">> > < & |", NULL);
    while (!feof(stdin))
    {
        printf("::$ ");
        user_input = scan_command();
        command = tokenize_string(user_input, separators);
        free(user_input);
        perform_command(list_to_argv(&command));
    }
    puts("\n-----");
    list_free_no_words(separators);
    return 0;
}
