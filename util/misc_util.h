#ifndef __MISC_UTIL_H__
#define __MISC_UTIL_H__

#include <limits.h>
#include <inttypes.h>
#include <ctype.h>
#include <errno.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>



int check_string(char *src, int size, int (*is_something)(int c));
int valid_money(char *amount, int *result);
#endif