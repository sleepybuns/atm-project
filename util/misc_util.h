#ifndef __MISC_UTIL_H__
#define __MISC_UTIL_H__
#include <stdlib.h>
#include <limits.h>
#include <inttypes.h>
#include <ctype.h>
#include <errno.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

int check_string(char *src, int size, int (*is_something)(int c));
int valid_money(char *amount, int *result);
int encrypt_aes256_cbc(unsigned char *plaintext, int plaintext_len, unsigned char *key,
                    unsigned char *iv, unsigned char *ciphertext);
int decrypt_aes256_cbc(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
                    unsigned char *iv, unsigned char *plaintext);
int handleErrors(void);
int digest_message(const unsigned char *message, size_t message_len, 
                    unsigned char *digest, const EVP_MD *md_type);
#endif