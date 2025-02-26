#ifndef CLEMULATOR_ARGV_PROCESSING_H
#define CLEMULATOR_ARGV_PROCESSING_H
#include "list.h"

typedef struct command_modifier
{
    int is_daemon;
    char* redirect_in;
    char* redirect_out;
    int append;
} command_modifier;

char** list_to_argv(list** head);
void free_argv(char** argv);
int get_argc(char** argv);
int argv_contains(char* argv[], const char* match_str);
int argv_count_entries(char* argv[], const char* match_str);
void remove_from_argv(char* argv[], int idx);
int retrieve_from_argv(char* argv[], const char* match_str);
int is_keyword(char* match_str);
int is_argv_valid(char* argv[]);
int is_argv_cdvalid(char* argv[]);
int is_piped_valid(char*** piped, int num_pipes);
int count_pipes(char* argv[]);
char*** pipe_split_argv(char* argv[]);
void free_piped_argv(char*** piped_argv, int num_pipes);
void print_piped_argv(char*** piped_argv, int num_pipes);
int get_unpiped_daemon(char* argv[]);
char* get_unpiped_redirect_filename(char* argv[], char* token);
command_modifier get_command_modifier(char* argv[]);
char** unjunk_command(char* argv[], list* separators);

#endif
