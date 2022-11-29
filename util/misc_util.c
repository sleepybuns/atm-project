#include "misc_util.h"

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

/*  Takes a string and parces as an int and saves it to result.
//  If the number represented by the string can fit into a signed int then 
//  return 1, else return 0.
//  note:   "12345xyz99"  will yield "12345" 
            "0000012345"  will yield "12345"
            "4000000000"  will error and return 0;
*/
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

/*  
//  This encryption function encrypts using the protocol and mode defined by 
//  the EVP_CIPHER struct that enc_type_mode points to.
//  This assumes iv and key have valid length for the encryption type.
//  
//  RETURNS ciphertext length on success, and 0 on failure.
//   
//  Ensure the buffer passed is big enough given the plaintext length.
*/
int encrypt_aes256_cbc (unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext) {
    EVP_CIPHER_CTX *ctx;
    int len, ciphertext_len;
    
    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new())) {
        return handleErrors();
    }

    /* Initialise the encryption operation. */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
       return handleErrors();
    }
    
    // Provide the message to be encrypted, and obtain the encrypted output.
    // EVP_EncryptUpdate can be called multiple times if necessary
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
        return handleErrors();
    }
    ciphertext_len = len;
    
    // Finalise the encryption. Further ciphertext bytes may be written at this stage.
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
        return handleErrors();
    }
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}

/*  
//  This decryption function decrypts a ciphertext that uses the protocol and 
//  mode defined by the EVP_CIPHER struct that enc_type_mode points to.
//  This assumes iv and key have valid length for the encryption type.
//  
//  RETURNS plaintext length on success, and 0 on failure.
//  plaintext is not null terimnated. Ensure the buffer passed is big enough.
*/
int decrypt_aes256_cbc (unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext) {
    EVP_CIPHER_CTX *ctx;
    int len, plaintext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new())) {
        return handleErrors();
    }
    /* Initialise the decryption operation. */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
        return handleErrors();
    }
    // Provide the message to be decrypted, and obtain the plaintext output.
    // EVP_DecryptUpdate can be called multiple times if necessary.
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
        return handleErrors();
    }
    plaintext_len = len;

    // Finalise the decryption. Further plaintext bytes may be written at this stage.
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
        return handleErrors();
    }
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);
    return plaintext_len;
}

int handleErrors(void) {
    ERR_print_errors_fp(stderr);
    return 0;
}

/*  Secure hash function. Takes in a message, and it's length, a buffer and a 
//  EVP_MD struct pointer and stores the result in the buffer and outputs the 
//  length of that hash. The length of the hash will conform to the length 
//  defined by the EVP_MD struct. 
//  RETURNS length of the hash on success, 0 on failure. 
*/
int digest_message(const unsigned char *message, size_t message_len, 
                    unsigned char *digest, const EVP_MD* md_type) {
	EVP_MD_CTX *mdctx;
    unsigned int digest_len;

	if((mdctx = EVP_MD_CTX_new()) == NULL) {
        return  handleErrors();
    }
	if(1 != EVP_DigestInit_ex(mdctx, md_type, NULL)) {
		return handleErrors();
    }
	if(1 != EVP_DigestUpdate(mdctx, message, message_len)) {
		return handleErrors();
    }
	if(1 != EVP_DigestFinal_ex(mdctx, digest, &digest_len)) {
		return handleErrors();
    }
	EVP_MD_CTX_free(mdctx);
    return digest_len;
}
