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

#include "argv_util.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

/* action on signal SIGCHLD */
void remove_zombies(int n)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
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

int wait_pid_arr(int* pids, int size)
{
    int i, term_pid, not_finished = 0;
    term_pid = wait(NULL);
    for (i = 0; i < size; i++)
    {
        pids[i] = (pids[i] == term_pid ? -1 : pids[i]);
        if (pids[i] != -1)
            not_finished = 1;
    }
    return not_finished;
}

/* unhandled cd */
void perform_pipe(char*** piped, int num_pipes, command_modifier cmd_mod)
{
    int* pids;
    int fd[2];
    int saved_fd = -1, i;
    pids = malloc(num_pipes * sizeof(*pids));
    for (i = 0; i < num_pipes - 1; i++)
    {
        pipe(fd);
        if ((pids[i] = fork()) == 0)
        {
            cmd_mod.redirect_out = NULL;
            close(fd[0]);
            if (i != 0)
            {
                dup2(saved_fd, 0);
                close(saved_fd);
                cmd_mod.redirect_in = NULL;
            }
            dup2(fd[1], 1);
            perform_command_w_pid(piped[i], pids[i], cmd_mod);
            perror(piped[i][0]);
            exit(1);
        }
        if (saved_fd != -1)
        {
            close(saved_fd);
        }
        saved_fd = fd[0];
        close(fd[1]);
    }
    if ((pids[num_pipes - 1] = fork()) == 0)
    {
        close(fd[1]);
        dup2(saved_fd, 0);
        cmd_mod.redirect_in = NULL;
        perform_command_w_pid(piped[num_pipes - 1], pids[num_pipes - 1],
                              cmd_mod);
        perror(piped[num_pipes - 1][0]);
        exit(1);
    }
    close(fd[0]);
    close(fd[1]);
    close(saved_fd);
    if (!cmd_mod.is_daemon)
    {
        while (wait_pid_arr(pids, num_pipes))
            ;
    }
    free(pids);
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
