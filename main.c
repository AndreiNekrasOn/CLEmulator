#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "argv_util.h"
#include "list.h"
#include "process_util.h"
#include "string_util.h"
#include "parser.h"

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
