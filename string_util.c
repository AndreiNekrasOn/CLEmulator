#include <stdlib.h>

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


