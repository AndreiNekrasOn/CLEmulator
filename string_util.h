#ifndef CLEMULATOR_STRING_UTIL
#define CLEMULATOR_STRING_UTIL

enum
{
    default_string_cap = 32
};

/* str memory and input */
char* reallocate_str(char* str, int size, int* capacity);
char* update_str(char* str, char symbol, int* size, int* capacity);
#endif
