#include "misc_util.h"
#include <stdlib.h>

// check if every char in string is true by is_something function
// return 0 if any char is false, 1 if all are true
int check_string(char *src, int size, int (*is_something)(int c)) {
    int idx;

    for (idx = 0; idx < size; idx++) {
        if (is_something(src[idx]) == 0) {
            return 0;
        }
    }
    return 1;
}

int valid_money(char *amount, int *result) {
    long int val;

    errno = 0;
    val = strtoimax(amount, NULL, 10);
    if (errno != 0 || val > INT_MAX) {
        return 0;
    }
    *result = (int)val;
    return 1;
}

