#ifndef CLEMULATOR_PARSER_H
#define CLEMULATOR_PARSER_H

#include "list.h"

enum separator_type
{
    not_separator,
    space,
    daemon_sep,
    redirect_stdout,
    redirect_stdout_a,
    redirect_stdin,
    pipe_line
};

enum separator_type identify_separator(char* separator);
enum separator_type check_separator_list(const char* str_i,
                                         const list* separators,
                                         int quote_flag);
char* get_separator_value_by_type(enum separator_type st);
void update_word(int* new_word_flag, list** tail, list** head,
                 const char* symbols, int len, int* word_size,
                 int* word_cap);
void mutate_to_default(int* word_size, int* word_cap, int* quote_flag,
                       int* new_word_flag);
list* tokenize_string(const char* str, list* separators);
#endif
