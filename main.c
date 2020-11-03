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
    redirect_stdout_a,
    redirect_stdin,
    pipe_line
};

typedef struct command_modifier
{
    int is_daemon;
    char* redirect_in;
    char* redirect_out;
    int append; /* -1 if redirect_in == NULL ? */
} command_modifier;

command_modifier get_command_modifier(char* argv[])
{
    command_modifier cm;
    cm.is_daemon = get_unpiped_daemon(argv);
    cm.redirect_in = get_unpiped_redirect_filename(argv, "<");
    cm.redirect_out = get_unpiped_redirect_filename(argv, ">");
    cm.append = -1;
    if (cm.redirect_out == NULL)
    {
        cm.redirect_out = get_unpiped_redirect_filename(argv, ">>");
        cm.append = 1;
    }
    else
    {
        cm.append = 0;
    }
    return cm;
}

char** unjunk_command(char* argv[], list* separators)
{
    int i;

    for (i = 0; argv[i] != NULL; i++)
    {
        if (strcmp(argv[i], "|") != 0 && list_has(separators, argv[i]))
        {
            free(argv[i]);
            argv[i] = NULL;
        }
    }
    return argv;
}

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
    {
        perror(filename);
        return -1;
    }
    dup2(fd, strem_fd);
    close(fd);
    return 0;
}

int check_and_perform_redirect(char* argv[], command_modifier cmd_mod)
{
    int success = 1;
    if (cmd_mod.redirect_in != NULL)
    {
        success &= !perform_redirect(cmd_mod.redirect_in, 0,
                                     O_RDONLY | O_BINARY);
    }
    if (cmd_mod.redirect_out != NULL)
    {
        if (cmd_mod.append == 0)
        {
            success &= !perform_redirect(cmd_mod.redirect_out, 1,
                                         O_WRONLY | O_CREAT | O_TRUNC
                                             | O_BINARY);
        }
        else
        {
            success &= !perform_redirect(cmd_mod.redirect_out, 1,
                                         O_WRONLY | O_APPEND | O_BINARY);
        }
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

void perform_command_w_pid(char* argv[], int pid, command_modifier cmd_mod)
{
    if (!pid)
    {
        if (check_and_perform_redirect(argv, cmd_mod) == -1)
        {
            fprintf(stderr, "Cannot perform redirection\n");
            exit(1);
        }
        execvp(argv[0], argv);
        perror(argv[0]);
        exit(1);
    }
}

void perform_single_command(char** argv, command_modifier cmd_mod)
{
    int pid;
    if (argv_contains(argv, "cd") == 0)
        perform_cd_command(argv[1]);
    else
    {
        pid = fork();
        perform_command_w_pid(argv, pid, cmd_mod);
        if (!cmd_mod.is_daemon)
            while (wait(NULL) != pid)
                ;
    }
}

int is_any_child_alive(int* pids, int size)
{
    int i;
    for (i = 0; i < size; i++)
    {
        if (pids[i] != -1)
            return 1;
    }
    return 0;
}

void stay_dead_and_out_of_this_world(int* pids, int size, int chld_pid)
{
    int i;
    for (i = 0; i < size; i++)
    {
        if (pids[i] == chld_pid)
        {
            pids[i] = -1;
            return;
        }
    }
}

void perform_pipe(char*** piped, int num_pipes, command_modifier cmd_mod)
{
    int pids[2];
    int fd[2];
    pipe(fd);
    if ((pids[0] = fork()) == 0) /* writes to pipe */
    {
        close(fd[0]);
        dup2(fd[1], 1);
        cmd_mod.redirect_out = NULL;
        perform_command_w_pid(piped[0], pids[0], cmd_mod);
        perror(piped[0][0]);
        exit(1);
    }
    if ((pids[1] = fork()) == 0) /* reads from pipe */
    {
        close(fd[1]);
        dup2(fd[0], 0);
        cmd_mod.redirect_in = NULL;
        perform_command_w_pid(piped[1], pids[1], cmd_mod);
        perror(piped[1][0]);
        exit(1);
    }
    close(fd[0]);
    close(fd[1]);
    if (cmd_mod.is_daemon)
        while (is_any_child_alive(pids, 2))
            stay_dead_and_out_of_this_world(pids, 2, wait(NULL));
}

void _perform_pipe(char*** piped, int num_pipes, command_modifier cmd_mod)
{
    int* pids;
    int fd[2];
    int saved_fd = -1, i;
    pids = malloc(num_pipes * sizeof(*pids));

    printf("numpipes=%d\n", num_pipes);
    for (i = 0; i < num_pipes - 1; i++)
    {
        pipe(fd);

        if ((pids[i] = fork()) == 0)
        {
            close(fd[0]);
            if (i != 0)
                dup2(saved_fd, 0);
            dup2(fd[1], 1);
            /*
            if (i != 0) cmd_mod.redirect_in = NULL;
            cmd_mod.redirect_out = NULL;
            */
            perform_command_w_pid(piped[i], pids[i], cmd_mod);
            perror(piped[i][0]);
            exit(1);
        }
        saved_fd = fd[0];
    }
    if ((pids[num_pipes - 1] = fork()) == 0)
    {
        close(fd[1]);
        dup2(fd[0], 0);
        /* cmd_mod.redirect_in = NULL; */
        perform_command_w_pid(piped[num_pipes - 1], pids[num_pipes - 1],
                              cmd_mod);
        perror(piped[num_pipes - 1][0]);
        exit(1);
    }
    if (cmd_mod.is_daemon)
        while (is_any_child_alive(pids, num_pipes))
            stay_dead_and_out_of_this_world(pids, num_pipes, wait(NULL));
}

/* assumes argv is valid */
void perform_command(char* argv[], command_modifier cmd_mod)
{
    char*** piped;
    int num_pipes;
    num_pipes = count_pipes(argv);
    piped = pipe_split_argv(argv);
    if (num_pipes == 1)
        perform_single_command(piped[0], cmd_mod);
    else if (is_piped_valid(piped, num_pipes))
        perform_pipe(piped, num_pipes, cmd_mod);
    free(piped);
}

void process_input(char* input, list* separators)
{
    list* command;
    char** cmd_argv;
    command_modifier modifier;
    command = tokenize_string(input, separators);
    cmd_argv = list_to_argv(&command);
    if (is_argv_valid(cmd_argv))
    {
        modifier = get_command_modifier(cmd_argv);
        cmd_argv = unjunk_command(cmd_argv, separators);
        perform_command(cmd_argv, modifier);
    }
    free_argv(cmd_argv);
}

/* main method */
int main(int argc, char* argv[])
{
    char* user_input;
    list* separators;
    signal(SIGCHLD, remove_zombies);
    separators = tokenize_string(">> > < & |", NULL);
    while (!feof(stdin))
    {
        printf("::$ ");
        user_input = scan_command();
        process_input(user_input, separators);
        free(user_input);
    }
    puts("\n-----");
    list_free_no_words(separators);
    return 0;
}
