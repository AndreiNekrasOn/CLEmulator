#ifndef CLEMULATOR_ARGV_PROCESSING_H
#define CLEMULATOR_ARGV_PROCESSING_H
#include "list.h"

char** list_to_argv(list** head);
void free_argv(char** argv);
int get_argc(char** argv);
int argv_contains(char* argv[], const char* match_str);
int argv_count_entries(char* argv[], const char* match_str);
void remove_from_argv(char* argv[], int idx);
int retrieve_from_argv(char* argv[], const char* match_str);
int is_keyword(char* match_str);
int is_argv_valid(char* argv[]);

#endif
