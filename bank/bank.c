#include "bank.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "../util/misc_util.h"
#include <openssl/rand.h>


Bank* bank_create(char *data) {
    
    Bank *bank = (Bank*) malloc(sizeof(Bank));
    if(bank == NULL) {
        perror("Could not allocate Bank");
        exit(1);
    }
    
    // Set up the network state
    bank->sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&bank->rtr_addr,sizeof(bank->rtr_addr));
    bank->rtr_addr.sin_family = AF_INET;
    bank->rtr_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    bank->rtr_addr.sin_port=htons(ROUTER_PORT);

    bzero(&bank->bank_addr, sizeof(bank->bank_addr));
    bank->bank_addr.sin_family = AF_INET;
    bank->bank_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    bank->bank_addr.sin_port = htons(BANK_PORT);
    bind(bank->sockfd,(struct sockaddr *)&bank->bank_addr,sizeof(bank->bank_addr));
   
    //set symetric key
    memcpy(bank->key, data, KEY_LEN);
    // Set up the protocol state
    bank->accounts = hash_table_create(100);
    bank->acc_nums = hash_table_create(100);
    bank->logged_user = NULL;
    memset(bank->active_card, 0, CARD_LEN + 1);
    bank->session_state = 0;
    bank->last_mssg_time = 0;

    return bank;
}

void bank_free(Bank *bank) {
    if(bank != NULL) {
        close(bank->sockfd);
        hash_table_free(bank->accounts);
        hash_table_free(bank->acc_nums);
        free(bank);
    }
}

ssize_t bank_send(Bank *bank, unsigned char *data, size_t data_len) {
    // Returns the number of bytes sent; negative on error
    return sendto(bank->sockfd, data, data_len, 0,
                  (struct sockaddr*) &bank->rtr_addr, sizeof(bank->rtr_addr));
}

ssize_t bank_recv(Bank *bank, unsigned char *data, size_t max_data_len) {
    // Returns the number of bytes received; negative on error
    return recvfrom(bank->sockfd, data, max_data_len, 0, NULL, NULL);
}

void bank_process_local_command(Bank *bank, char *command, size_t len) {
    char subcmd[DATASIZE + 1] = "";
    int index = 0;

    sscanf(command, " %s %n", subcmd, &index);
    command += index; //command points to next non-whitespace character

    if (!strcmp(subcmd, "create-user")) {
        char username[DATASIZE + 1] = "", pin[DATASIZE + 1] = "";
        char balance[DATASIZE + 1] = "", card_name[DATASIZE + 1] = "";
        char *bank_user, *card_num;
        int init_bal;
        size_t user_len;
        Userfile *user_file;
        FILE *output;

        index = 0;
        sscanf(command, "%s %s %s %n", username, pin, balance, &index);

        // check validitiy of arguments
        // index == 0 means there aren't 3 arguments
        // *(command + index) is truthy if there are more input after
        // check the validity of the characters in all args
        // check balance can fit into int. side effect: value stored in init_bal
        if (index == 0 || *(command + index) || 
            (user_len = strlen(username)) > MAX_USERNAME_LEN || strlen(pin) != PIN_LEN ||
            check_string(username, user_len, isalpha) == 0 || 
            check_string(pin, PIN_LEN, isdigit) == 0 ||
            check_string(balance, strlen(balance), isdigit) == 0 ||
            valid_money(balance, &init_bal) == 0) {
            
            printf("Usage:  create-user <user-name> <pin> <balance>\n");
            return;
        }
        if (hash_table_find(bank->accounts, username)) {
            printf("Error:  user %s already exists\n", username);
            return;
        }
        // create a <username>.card file
        strcpy(card_name, username);
        strcat(card_name, ".card");
        if ((output = fopen(card_name, "w")) == NULL) {
            printf("Error creating card file for user %s\n", username);
            return;
        }
        // create card number
        strncat(card_name, pin, PIN_LEN);
        card_num = calloc(CARD_LEN + 1, sizeof(char)); 
        gen_card_num(bank, card_num, (unsigned char*) card_name, strlen(card_name));
        fputs(card_num, output); 
        fclose(output);

        //add the account in the hashtable
        user_file = (Userfile *) malloc(sizeof(Userfile));
        strncpy(user_file->pin, pin, PIN_LEN);
        strncpy(user_file->card_num, card_num, CARD_LEN);
        user_file->balance = init_bal;
        bank_user = calloc(user_len + 1, sizeof(char));
        strncpy(bank_user, username, user_len);
        hash_table_add(bank->accounts, bank_user, user_file);
        hash_table_add(bank->acc_nums, card_num, bank_user);
        printf("Created user %s\n", username);

    } else if (!strcmp(subcmd, "deposit")) {
        char username[DATASIZE + 1] = "", amt[DATASIZE + 1] = "";
        int deposit, user_bal;
        size_t user_len;
        Userfile *target;

        index = 0;
        sscanf(command, "%s %s %n", username, amt, &index);
        //sanitize input
        if (index == 0 || *(command + index) || 
            (user_len = strlen(username)) > MAX_USERNAME_LEN || 
            check_string(username, user_len, isalpha) == 0 || 
            check_string(amt, strlen(amt), isdigit) == 0 ||
            valid_money(amt, &deposit) == 0) {
            
            printf("Usage:  deposit <user-name> <amt>\n");
            return;
        }
        if ((target = (Userfile*)hash_table_find(bank->accounts, username)) == NULL) {
            printf("No such user\n");
            return;
        }
        
        user_bal = target->balance;
        if (INT_MAX - user_bal < deposit) {
            printf("Too rich for this program\n");
            return;
        }
        target->balance = user_bal + deposit;
        printf("$%d added to %s's account\n", deposit, username);

    } else if (!strcmp(subcmd, "balance")) {
        char username[DATASIZE + 1] = "";
        size_t user_len;
        Userfile *target;

        index = 0;
        sscanf(command, "%s %n", username, &index);
        //sanitize input
        if (index == 0 || *(command + index) || 
            (user_len = strlen(username)) > MAX_USERNAME_LEN || 
            check_string(username, user_len, isalpha) == 0) {
            printf("Usage:  balance <user-name>\n");
            return;
        }
        if ((target = (Userfile*)hash_table_find(bank->accounts, username)) == NULL) {
            printf("No such user\n");
            return;
        }
        printf("$%d\n", target->balance);

    } else {
        printf("Invalid command\n");
    }
}

void bank_process_remote_command(Bank *bank, unsigned char *recv_data, size_t len) {
    unsigned char iv[IV_LEN], ciphertext[DATASIZE + 1] = "";
    unsigned char plaintext[DATASIZE + 1] = "";
    char curr_card_num[CARD_LEN + 1], *plain_ptr = (char*) plaintext;
    char reply[DATASIZE + 1] = "";
    long message_time = 0, cpu_cycle = 0;
    int plaintext_len, cipher_len, reply_len;

    // extract IV
    memcpy(iv, recv_data, IV_LEN); 
    recv_data += IV_LEN;

    // decrypte message
    plaintext_len = decrypt_aes256_cbc(recv_data, len - IV_LEN, bank->key, iv, plaintext);
    if (plaintext_len == 0) {
        return;
    } 
    // extract time stamps
    memcpy(&message_time, plain_ptr, sizeof(long));
    memcpy(&cpu_cycle, plain_ptr + sizeof(long), sizeof(long));
    plain_ptr += (sizeof(long) * 2);
    plaintext_len -= (sizeof(long) * 2);
    // check time stamps
    if (message_time < bank->last_mssg_time) {
        return;
    } else if (message_time == bank->last_mssg_time && cpu_cycle <= bank->last_cpu_cycle) {
        return;
    }
    // check card number
    strncpy(curr_card_num, plain_ptr, CARD_LEN);
    if (!strcmp(curr_card_num, bank->active_card)) {
        return;
    }
    reply_len = process_msg_helper(bank, plain_ptr, plaintext_len, reply, DATASIZE + 1);

    do {
        if (RAND_priv_bytes(iv, IV_LEN) <= 0) {
            continue;
        }
        cipher_len = encrypt_aes256_cbc(reply, reply_len, bank->key, iv, ciphertext);

        if (cipher_len == 0) {
            continue;
        } else {
            unsigned char message[cipher_len + IV_LEN];

            memcpy(message, iv, IV_LEN);
            memcpy(message + IV_LEN, ciphertext, cipher_len);
            bank_send(bank, message, cipher_len + IV_LEN);
            return;
        }

    } while (1);
    
}

int process_mssg_helper(Bank *bank, char *command, int cmd_len, char *reply, int reply_buf_size) {
    char subcmd[DATASIZE + 1] = "";

    
}

void gen_card_num(Bank *bank, char *card_num, unsigned char *plaintext, int plaintext_len) {
    unsigned char iv[IV_LEN], ciphertext[DATASIZE], hash[HASH_LEN];
    char lil_buff[3] = "";
    int cipher_len = 0, hash_len, idx;

    do {
        if (RAND_priv_bytes(iv, IV_LEN) <= 0) {
            continue;
        }
        cipher_len = encrypt_aes256_cbc(plaintext, plaintext_len, bank->key, iv, ciphertext);
        if (cipher_len == 0) {
            continue;
        }
        hash_len = digest_message(ciphertext, cipher_len, hash, EVP_md5());
        if (hash_len == 0) {
            continue;
        }
        for (idx = 0; idx < hash_len; idx++) {
            sprintf(lil_buff, "%02x", hash[idx]);
            strncat(card_num, lil_buff, 2);
        }
        if (hash_table_find(bank->acc_nums, card_num)){
            continue;
        }
        return;
    } while (1);

}

int construct_payload (Bank *bank, char *payload, char *text, int text_len) {
    time_t curr_time = time(&(bank->last_mssg_time));
    clock_t curr_cpu_cycle = clock();

    memcpy(payload, (void*) curr_time, sizeof(long));
    memcpy(payload + sizeof(long), (void*) curr_cpu_cycle, sizeof(long));
    payload += (sizeof(long) * 2);
    memcpy(payload, bank->active_card, CARD_LEN);
    payload += CARD_LEN;
    memcpy(payload, text, text_len);
    payload[text_len] = '\0';
    return (sizeof(long) * 2) + CARD_LEN + text_len + 1;
}

void close_session(Bank *bank) {
    bank->logged_user = NULL;
    memset(bank->active_card, 0, CARD_LEN + 1);
    bank->session_state = 0;
}

