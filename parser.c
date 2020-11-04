#include <ctype.h>
#include <stdio.h>
#include <string.h> 

#include "parser.h"
#include "string_util.h"


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
        return daemon_sep;
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
    case daemon_sep:
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

