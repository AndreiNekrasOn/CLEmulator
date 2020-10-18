#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <regex.h>

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
    redirect_stdout_a
};

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
regex_t get_parser_regex()
{
    regex_t result;
    const char* regex_str = "\"[^\"]+\"|[^ ><&\"]+|&|>>|>|<|\"";
    if (regcomp(&result, regex_str, REG_EXTENDED))
    {
        perror("Could not compile regex");
        exit(1);
    }
    return result;
}

list* tokenize_string(const char* str, regex_t regex_compiled)
{
    regmatch_t match_info;
    char* word;
    list* head = NULL, *tail = NULL;
    int word_size, i;
    if (str == NULL)
        return NULL;
    for (i = 0;
         !regexec(&regex_compiled, str + i, 1, &match_info, 0);
         i += match_info.rm_eo)
    {
        word_size = match_info.rm_eo - match_info.rm_so;
        word = malloc(word_size + 1);
        strncpy(word, str + i + match_info.rm_so, word_size);
        word[word_size] = '\0';
        tail = list_insert(tail, word);
        if (head == NULL)
	    head = tail;
    }
    return head;
}

/* argv processing */
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
        free(argv[i]);
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

int argv_contains(char* argv[], const char* match_str)
{
    int i;
    for (i = 0; argv[i] != NULL; i++)
    {
        if (!strcmp(argv[i], match_str))
            return i;
    }
    return -1;
}

void remove_from_argv(char* argv[], int idx)
{
    for (; argv[idx + 1] != NULL; idx++)
        argv[idx] = argv[idx + 1];
    argv[idx] = NULL;
}

int retrieve_from_argv(char* argv[], const char* match_str)
{
    int i;
    i = argv_contains(argv, match_str);
    if (i == -1)
        return -1;
    remove_from_argv(argv, i);
    return i;
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

void perform_command(char** argv)
{
    int pid;
    int daemon_flag;
    if (argv_contains(argv, "cd") == 0)
        perform_cd_command(argv[1]);
    else
    {
        daemon_flag = is_daemon(argv);
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
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ; /* remove zombies */
}

/* main method */
int main()
{
    list* command;
    char** cmd_argv;
    regex_t separators_regex;
    char* user_input;
    separators_regex = get_parser_regex();
    while (!feof(stdin))
    {
        printf("::$ ");
        user_input = scan_command();
        command = tokenize_string(user_input, separators_regex);
        free(user_input);
        if (command == NULL)
            continue;
        /* list_print(command); */
        cmd_argv = list_to_argv(&command);
        perform_command(cmd_argv);
        free_argv(cmd_argv);
    }
    puts("\n-----");
    regfree(&separators_regex);
    return 0;
}
