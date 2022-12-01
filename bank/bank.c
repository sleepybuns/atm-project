#include "bank.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "../util/misc_util.h"
#include <openssl/rand.h>


Bank* bank_create(char *data) {
    struct timeval time;
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
    memset(bank->active_user, 0, MAX_USERNAME_LEN + 1);
    bank->session_state = 0;
    gettimeofday(&time, NULL);
    bank->last_msg_sec = time.tv_sec;
    bank->last_msg_usec = time.tv_usec;
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
            
            printf("Usage:  create-user <user-name> <pin> <balance>\n\n");
            return;
        }
        if (hash_table_find(bank->accounts, username)) {
            printf("Error:  user %s already exists\n\n", username);
            return;
        }
        // create a <username>.card file
        strcpy(card_name, username);
        strcat(card_name, ".card");
        if ((output = fopen(card_name, "w")) == NULL) {
            printf("Error creating card file for user %s\n\n", username);
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
        printf("Created user %s\n\n", username);

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
            
            printf("Usage:  deposit <user-name> <amt>\n\n");
            return;
        }
        if ((target = (Userfile*)hash_table_find(bank->accounts, username)) == NULL) {
            printf("No such user\n\n");
            return;
        }
        
        user_bal = target->balance;
        if (INT_MAX - user_bal < deposit) {
            printf("Too rich for this program\n\n");
            return;
        }
        target->balance = user_bal + deposit;
        printf("$%d added to %s's account\n\n", deposit, username);

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
            printf("Usage:  balance <user-name>\n\n");
            return;
        }
        if ((target = (Userfile*)hash_table_find(bank->accounts, username)) == NULL) {
            printf("No such user\n\n");
            return;
        }
        printf("$%d\n", target->balance);

    } else {
        printf("Invalid command\n\n");
    }
}

void bank_process_remote_command(Bank *bank, unsigned char *recv_data, size_t len) {
    unsigned char iv[IV_LEN], ciphertext[DATASIZE + 1] = "";
    unsigned char plaintext[DATASIZE + 1] = "", send_to_atm[DATASIZE + 1] = "";
    char curr_card_num[CARD_LEN + 1], *plain_ptr = (char*) plaintext;
    char reply_cmd[DATASIZE + 1];
    long msg_time_sec = 0;
    int msg_time_usec = 0, plaintext_len, cipher_len, reply_msg_len;

    // extract IV
    memcpy(iv, recv_data, IV_LEN); 
    recv_data += IV_LEN;

    // decrypte message
    if ((plaintext_len = decrypt_aes256_cbc(recv_data, len - IV_LEN, bank->key, iv, plaintext)) == 0) {
        return; //decrypt failed, maybe message was altered
    } 

    // extract time stamps
    memcpy(&msg_time_sec, plain_ptr, sizeof(long));
    memcpy(&msg_time_usec, plain_ptr + sizeof(long), sizeof(int));
    plain_ptr += (sizeof(long) + sizeof(int));
    plaintext_len -= (sizeof(long) + sizeof(int));

    // check time stamps
    if (msg_time_sec < bank->last_msg_sec) {
        return; 
    } else if (msg_time_sec == bank->last_msg_sec && msg_time_usec <= bank->last_msg_usec) {
        return;
    }
    
    if (bank->session_state == NO_SESH) { 
        char username[MAX_USERNAME_LEN + 1] = "";
        //extract username
        sscanf(plain_ptr, "%s", username);

        if (hash_table_find(bank->accounts, username)) {
            strcpy(reply_cmd, "user-found");
            bank->session_state = AWAIT_PIN;
            strcpy(bank->active_user, username); //set active user

        } else { // user not found
            strcpy(reply_cmd, "no-user-found");
            // bank state not changed
        }
        reply_msg_len = construct_response_initial(bank, send_to_atm, reply_cmd, strlen(reply_cmd)); // change

    } else {
        char subcmd[DATASIZE + 1] = "";
        int index = 0;

        strncpy(curr_card_num, plain_ptr, CARD_LEN); // extract card number
        plain_ptr += CARD_LEN;
    
        sscanf(plain_ptr, "%s %n", subcmd, &index);

        if (bank->session_state == AWAIT_PIN) {
            if (!strcmp(subcmd, "unverifiable")) {
                // reset bank state
                bank->session_state = NO_SESH;
                memset(bank->active_user, 0, MAX_USERNAME_LEN + 1);
                // save time stamps
                bank->last_msg_sec = msg_time_sec; 
                bank->last_msg_usec = msg_time_usec;
                return;

            } else if (!strcmp(subcmd, "verify")) { 
                char pin_verify[PIN_LEN + 1] = "", *verify_username;
                Userfile *curr_userfile;

                sscanf(plain_ptr + index, "%s", pin_verify);
                strcpy(bank->active_card, curr_card_num);// save card number
                
                // check card for the username
                verify_username = hash_table_find(bank->acc_nums, curr_card_num);

                if (verify_username == NULL || strcmp(verify_username, bank->active_user)) {
                    strcpy(reply_cmd, "access-denied");
                    reply_msg_len = construct_response(bank, send_to_atm, reply_cmd, strlen(reply_cmd));
                    memset(bank->active_card, 0, CARD_LEN + 1); // clear card
                    memset(bank->active_user, 0, MAX_USERNAME_LEN + 1); // clear user
                    bank->session_state = NO_SESH; 

                } else { // now to verify the pin
                    curr_userfile = hash_table_find(bank->accounts, verify_username); // get userfile

                    if (strcmp(pin_verify, curr_userfile->pin)) {
                        strcpy(reply_cmd, "access-denied");
                        reply_msg_len = construct_response(bank, send_to_atm, reply_cmd, strlen(reply_cmd));
                        memset(bank->active_card, 0, CARD_LEN + 1); // clear card
                        memset(bank->active_user, 0, MAX_USERNAME_LEN + 1); // clear user
                        bank->session_state = NO_SESH; 

                    } else { // pin is approved
                        strcpy(reply_cmd, "access-granted");
                        reply_msg_len = construct_response(bank, send_to_atm, reply_cmd, strlen(reply_cmd));
                        bank->session_state = OPEN_SESH;
                        bank->logged_user = curr_userfile; // save userfile in cache
                    }
                }
            } else {
                return;
            }

        } else {
            // check card number 
            if (strcmp(curr_card_num, bank->active_card)) {
                return; // card number does not match the active user
            }

            if (bank->session_state == 11) {
                if (!strcmp(subcmd, "withdraw")) {
                    int amt = 0;
                
                    memcpy(&amt, plain_ptr + strlen(subcmd) + 1, sizeof(int));

                    if (bank->logged_user->balance - amt < 0) {
                        strcpy(reply_cmd, "insufficient");
                        reply_msg_len = construct_response(bank, send_to_atm, reply_cmd, strlen(reply_cmd));

                    } else { // user has enough money
                        strcpy(reply_cmd, "dispense");
                        memcpy(reply_cmd + 9, &amt, sizeof(int));
                        // we are including the int after the command seperated by a NULL byte
                        reply_msg_len = construct_response(bank, send_to_atm, reply_cmd, strlen(reply_cmd) + 1 + sizeof(int));
                        bank->session_state = WITHDRAW;
                    }

                } else if (!strcmp(subcmd, "balance")) {
                    strcpy(reply_cmd, "balance");
                    memcpy(reply_cmd + 8, &(bank->logged_user->balance), sizeof(int));
                    // we are including the int after the command seperated by a NULL byte
                    reply_msg_len = construct_response(bank, send_to_atm, reply_cmd, strlen(reply_cmd) + 1 + sizeof(int));

                } else if (!strcmp(subcmd, "end-session")) {
                    // reset bank cache and state
                    bank->logged_user = NULL;
                    memset(bank->active_card, 0, CARD_LEN + 1);
                    memset(bank->active_user, 0, MAX_USERNAME_LEN + 1);
                    bank->session_state = NO_SESH;
                    // record time stamps
                    bank->last_msg_sec = msg_time_sec; 
                    bank->last_msg_usec = msg_time_usec;
                    return;

                } else {
                    return;

                }
            } else if (bank->session_state == 33) {
                if (!strcmp(subcmd, "dispensed")) {
                    int amt = 0;

                    memcpy(&amt, plain_ptr + strlen(subcmd) + 1, sizeof(int));
                    bank->logged_user->balance -= amt;
                    bank->session_state = OPEN_SESH;
                    // record time stamps
                    bank->last_msg_sec = msg_time_sec; 
                    bank->last_msg_usec = msg_time_usec;
                    return;

                } else {
                    return;
                }
            } else {
                return;
            }
        }
    }

    // encrypt reply and send
    do {
        if (RAND_priv_bytes(iv, IV_LEN) <= 0) {
            continue;
        }
        cipher_len = encrypt_aes256_cbc(send_to_atm, reply_msg_len, bank->key, iv, ciphertext);
        
        if (cipher_len == 0) {
            continue;
        } else {
            unsigned char payload[cipher_len + IV_LEN];

            memcpy(payload, iv, IV_LEN);
            memcpy(payload + IV_LEN, ciphertext, cipher_len);
            bank_send(bank, payload, cipher_len + IV_LEN);
            return;
        }
    } while (1);
    
}

void gen_card_num(Bank *bank, char *card_num, unsigned char *plaintext, int plaintext_len) {
    unsigned char iv[IV_LEN], ciphertext[DATASIZE], hash[HASH_LEN], zero[HASH_LEN];
    char lil_buff[3] = "";
    int cipher_len = 0, hash_len, idx;

    memset(zero, 0, HASH_LEN);
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
        if (memcmp(hash, zero, HASH_LEN) == 0) {
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

int construct_response (Bank *bank, unsigned char *response, char *command, int cmd_len) {
    struct timeval curr_time;

    gettimeofday(&curr_time, NULL);
    bank->last_msg_sec = curr_time.tv_sec; // save time stamps in bank cache
    bank->last_msg_usec = curr_time.tv_usec;

    // write time stamps into response
    memcpy(response, &(curr_time.tv_sec), sizeof(long));
    memcpy(response + sizeof(long), &(curr_time.tv_usec), sizeof(int));
    response += (sizeof(long) + sizeof(int));

    // write card number
    memcpy(response, bank->active_card, CARD_LEN);
    response += CARD_LEN;

    // write the response command
    memcpy(response, command, cmd_len);
    response[cmd_len] = '\0'; // add NULL byte at the end

    // '+ 1' at the end to include the NULL byte 
    return sizeof(long) + sizeof(int) + CARD_LEN + cmd_len + 1; 
}

int construct_response_initial (Bank *bank, unsigned char *response, char *command, int cmd_len) {
    struct timeval curr_time;

    gettimeofday(&curr_time, NULL);
    bank->last_msg_sec = curr_time.tv_sec; // save time stamps in bank cache
    bank->last_msg_usec = curr_time.tv_usec;

    // write time stamps into response
    memcpy(response, &(curr_time.tv_sec), sizeof(long));
    memcpy(response + sizeof(long), &(curr_time.tv_usec), sizeof(int));
    response += (sizeof(long) + sizeof(int));

    // write the response command
    memcpy(response, command, cmd_len);
    response[cmd_len] = '\0'; // add NULL byte at the end

    // '+ 1' at the end to include the NULL byte 
    return sizeof(long) + sizeof(int) + cmd_len + 1; 
}
