#include <stdio.h>
#include <stdlib.h>


enum {
    DEFAULT_STRING_CAP = 32
};

typedef struct list {
    char *word;
    struct list *next;
} list;

char *reallocate_str(char *str, int size, int *capacity) {
    if (size + 1 == *capacity) {
        *capacity *= 2;
        str = realloc(str, *capacity);
    }
    return str;
}

list *list_init() {
    list *l = malloc(sizeof(*l));
    l->next = NULL;
    l->word = NULL;
    return l;
}

void list_print(const list *head) {
    if (head == NULL) return;

    if (head->word == NULL) exit(69);
    printf("%s\n", head->word);
    list_print(head->next);
}

void list_free(list *head) {
    if (head == NULL) return;
    list *next = head->next;
    free(head->word);
    free(head);
    list_free(next);
}

list *list_insert_symbol(list *tail, char symbol, int *size, int *capacity) {
    if (tail->word == NULL) { /* word is not initiated */
        tail->word = malloc(DEFAULT_STRING_CAP);
    }
    tail->word = reallocate_str(tail->word, *size, capacity);

    tail->word[(*size)++] = symbol;
    tail->word[(*size)] = '\0';
    return tail;
}

list *list_insert(list *tail) {
    if (tail == NULL) {
        return list_init();
    }
    list *child = list_init();
    tail->next = child;
    return child;
}


void mutate_to_default(int *size, int *cap, int *quote_flag, int *new_word_flag) {
    (*size) = 0;
    (*cap) = DEFAULT_STRING_CAP;
    (*quote_flag) = 0;
    (*new_word_flag) = 1;
}

void command_end_action(list **head, list **tail, int *size, int *capacity, int *quote_flag, int *new_word_flag) {
    if (*quote_flag)
        printf("Error: unclosed '\"'.\n");
    else {
        list_print(*head);
    }
    list_free(*head);
    mutate_to_default(size, capacity, quote_flag, new_word_flag);
    *head = *tail = NULL;
}

void print_prompt() {
    printf(">>");
}

int main() {
    int symbol;
    int quote_flag = 0;
    int new_word_flag = 1;
    int size, capacity;

    list *head = NULL;
    list *tail = NULL;

    mutate_to_default(&size, &capacity, &quote_flag, &new_word_flag);

    print_prompt();
    while ((symbol = getchar()) != EOF) {
        if (symbol == '\n') {
            command_end_action(&head, &tail, &size, &capacity, &quote_flag, &new_word_flag);
	    print_prompt();
        } else if (symbol == ' ' && !quote_flag) {
            if (size == 0) continue;
            mutate_to_default(&size, &capacity, &quote_flag, &new_word_flag);
        } else {
            if (new_word_flag) {
                new_word_flag = 0;
                tail = list_insert(tail);
            }
            if (symbol == '"') quote_flag = !quote_flag;
            tail = list_insert_symbol(tail, (char) symbol, &size, &capacity);
            if (head == NULL) { /* first word in the command is here*/
                head = tail;
            }
        }
    }
    command_end_action(&head, &tail, &size, &capacity, &quote_flag, &new_word_flag);
    printf("\n"); 
    return 0;
}
