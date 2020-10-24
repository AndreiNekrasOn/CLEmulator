#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    int i, size;
    size = strlen(match_str);
    for (i = 0; argv[i] != NULL; i++)
    {
        if (strlen(argv[i]) == size && !strncmp(argv[i], match_str, size))
            return i;
    }
    return -1;
}

int argv_count_entries(char* argv[], const char* match_str)
{
    int i, size, res = 0;
    size = strlen(match_str);
    for (i = 0; argv[i] != NULL; i++)
    {
        if (strlen(argv[i]) == size && !strncmp(argv[i], match_str, size))
            res++;
    }
    return res;
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

int is_keyword(char* match_str)
{
    char* specials[5];
    int i, res = 0;
    specials[0] = "&";
    specials[1] = ">";
    specials[2] = "<";
    specials[3] = ">>";
    specials[4] = "|";
    for (i = 0; i < 5; i++)
    {
        res |= strlen(specials[i]) == strlen(match_str) && !strncmp(match_str, specials[i], strlen(specials[i]));
    }
    return res || (match_str == NULL);
}

int is_argv_valid(char* argv[])
{
    int argc, position, i, is_daemon = 0;
    char* redirect_words[3];
    redirect_words[0] = ">", redirect_words[1] = "<",
    redirect_words[2] = ">>";
    argc = get_argc(argv);
    if (argv_contains(argv, "cd") != -1
        && (argc != 2 || is_keyword(argv[1])))
    {
        fprintf(stderr, "Bad argument for cd\n");
        return 0;
    }
    if ((position = argv_contains(argv, "&")) != -1
        && (position != argc - 1 || argc == 1))
    {
        fprintf(stderr, "Incorrect '&' position\n");
        return 0;
    }
    is_daemon = (position == argc - 1);

    if (argv_contains(argv, ">") != -1 && argv_contains(argv, ">>") != -1)
    {
        fprintf(stderr, "Unclear redirection: mixed write/append\n");
        return 0;
    }
    for (i = 0; i < 3; i++)
    {
        if (argv_count_entries(argv, redirect_words[i]) > 1)
        {
            fprintf(stderr, "Double stream redirection\n");
            return 0;
        }
        position = argv_contains(argv, redirect_words[i]);
        if (position == -1)
        {
            continue;
        }
        if (position == 0
            || !(position == argc - 2 - is_daemon
                 || (position + 2 < argc - is_daemon
                     && (argv[position + 2] == redirect_words[(i + 1) % 3]
                         || argv[position + 2]
                             == redirect_words[(i + 2) % 3]))))
        {
            fprintf(stderr, "Invalid redirect keyword position\n");
            return 0;
        }
        if (is_keyword(argv[position + 1]))
        {
            fprintf(stderr, "Filename is empty or is a keyword\n");
            return 0;
        }
    }
    return 1;
}
