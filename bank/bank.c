#include "bank.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>


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
   
    //set sysmetric key
    memcpy(bank->key, data, KEY_LEN);
    // Set up the protocol state
    bank->accounts = hash_table_create(100);
    bank->logged_user = NULL;
    memset(bank->active_card, 0, CARD_LEN + 1);
    memset(bank->active_user, 0, MAX_USERNAME_LEN + 1);
    bank->session_state = 0;

    return bank;
}

void bank_free(Bank *bank) {
    if(bank != NULL) {
        close(bank->sockfd);
        hash_table_free(bank->accounts);

        free(bank);
    }
}

ssize_t bank_send(Bank *bank, char *data, size_t data_len) {
    // Returns the number of bytes sent; negative on error
    return sendto(bank->sockfd, data, data_len, 0,
                  (struct sockaddr*) &bank->rtr_addr, sizeof(bank->rtr_addr));
}

ssize_t bank_recv(Bank *bank, char *data, size_t max_data_len) {
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
        char *bank_user, card_num[CARD_LEN + 1] = "";
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
            
            printf("Usage:  create-user <user-name> <pin> <balance> \n");
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
        fputs(username, output);
        fputc('\n', output);
        //create card number 
        //sprintf(w_buffer, "%u", hash(function_encrypte(username), cyphertext_len)); 
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
            
            printf("Usage:  deposit <user-name> <amt> \n");
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
    unsigned char iv[IV_LEN];
    unsigned char ciphertext[DATASIZE + 1] = "";
    char plaintext[DATASIZE + 1] = "";
    char session_username[MAX_USERNAME_LEN + 1] = "";
    char session_card_num[CARD_LEN + 1] = "";
    int index;

    memcpy(iv, recv_data, IV_LEN); //store session IV;
    recv_data += IV_LEN;
    funct_decrypt(recv_data, len - IV_LEN, plaintext, bank->key, iv);
    sscanf(plaintext, "%s %s %n", session_card_num, session_username, &index);
    if (index == 0 || *(plaintext + index)) {
            // send error HERE
            //
        close_session(bank);
        return;
    }
	if (bank->session_state == 0) {
         

    } else {
        int cipher_len;
        

        


        cipher_len = funct_encrypt(plaintext, strlen(plaintext), ciphertext, bank->key, iv);
        bank_send(bank, ciphertext, cipher_len);
    }
}

void close_session(Bank *bank) {
    bank->logged_user = NULL;
    memset(bank->active_card, 0, CARD_LEN);
    memset(bank->active_user, 0, MAX_USERNAME_LEN);
    bank->session_state = 0;
}

void create_card_num(uint32_t hash, char *pin, char *card_num) {
    char arr[11] = "";
    int idx;

    arr[9] = '0';
    sprintf(arr, "%u", hash);
    for (idx = 0; idx < PIN_LEN; idx++) {
        
    }
}
