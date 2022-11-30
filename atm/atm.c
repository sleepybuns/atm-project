#include "atm.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <openssl/rand.h>

ATM* atm_create(char *data)
{
    struct timeval time; 
    ATM *atm = (ATM*) malloc(sizeof(ATM));
    if(atm == NULL)
    {
        perror("Could not allocate ATM");
        exit(1);
    }

    // Set up the network state
    atm->sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&atm->rtr_addr,sizeof(atm->rtr_addr));
    atm->rtr_addr.sin_family = AF_INET;
    atm->rtr_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    atm->rtr_addr.sin_port=htons(ROUTER_PORT);

    bzero(&atm->atm_addr, sizeof(atm->atm_addr));
    atm->atm_addr.sin_family = AF_INET;
    atm->atm_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    atm->atm_addr.sin_port = htons(ATM_PORT);
    bind(atm->sockfd,(struct sockaddr *)&atm->atm_addr,sizeof(atm->atm_addr));

    // No session initially
    memset(atm->curr_user, 0, MAX_USERNAME_LEN + 1);
    memset(atm->active_card, 0, CARD_LEN + 1);
    atm->session_state = INITIAL;
    gettimeofday(&time, NULL);
    atm->last_mssg_time = time.tv_sec;
    atm->last_mssg_micro_time = time.tv_usec;

    // set symmetric key 
    memcpy(atm->key, data, KEY_LEN);

    return atm;
}

void atm_free(ATM *atm)
{
    if(atm != NULL)
    {
        close(atm->sockfd);
        free(atm);
    }
}

ssize_t atm_send(ATM *atm, char *data, size_t data_len)
{
    // Returns the number of bytes sent; negative on error
    return sendto(atm->sockfd, data, data_len, 0,
                  (struct sockaddr*) &atm->rtr_addr, sizeof(atm->rtr_addr));
}

ssize_t atm_recv(ATM *atm, char *data, size_t max_data_len)
{
    // Returns the number of bytes received; negative on error
    return recvfrom(atm->sockfd, data, max_data_len, 0, NULL, NULL);
}

void atm_process_command(ATM *atm, char *command)
{
    // TODO: Implement the ATM's side of the ATM-bank protocol

    char action[DATASIZE + 1] = "", arg[DATASIZE + 1] = "", extra[DATASIZE + 1] = "";
    FILE *input;
    char recvline[DATASIZE + 1] = "", recvcommand[DATASIZE + 1];
    char pin[DATASIZE + 1] = "", user[MAX_USERNAME_LEN + 1] = "";
    unsigned char ciphertext[DATASIZE + 1] = "", plaintext[DATASIZE + 1] = ""; 
    char to_send[DATASIZE + 1] = "", command_option[DATASIZE + 1] = "";
    int n, values = 0, invalid = 0, i, amount, process_result = 0, recvarg;

    // DEBUG: printf(" String read in: %s\n", command);
    values = sscanf(command, "%s %s %s", action, arg, extra);

    if(strcmp(action, "begin-session") == 0) {
        // DEBUG: printf("BEGIN SESSION\n");

        if(strlen(atm->curr_user) == 0) { // checks for active session

            // check for any non-alphabetic characters
            for(i = 0; i < strlen(arg); i++) {
                if(!isalpha(arg[i])) {
                    invalid = 1; 
                }
            } 
            // check for invalid command input
            if(values != 2 || strlen(arg) > MAX_USERNAME_LEN || invalid) {
                printf("Usage: begin-session <user-name>\n");
            } else {
                
                // save username
                strncpy(user, arg, strlen(arg));
                user[strlen(arg)] = '\0';
                // DEBUG: printf("Checking user: %s\n", user);
    
                // check <user>.card file is present
                strcat(arg, ".card");
                if((input = fopen(arg, "r")) == NULL) { // looks in curr dir (bin)
                    printf("Unable to access %s's card\n", user);
                } else {

                    // store active card number 
                    fread(atm->active_card, 1, CARD_LEN, input);

                    do { // continues receiving messages until one is accepted
                        construct_message(atm, to_send, plaintext, ciphertext, "login-request", strlen("login-request"), -1);
                        atm_send(atm, to_send, strlen(to_send));
                        atm->session_state = LOGIN_REQ_WAITING;

                        // Get bank reply
                        n = atm_recv(atm, recvline, DATASIZE);
                        recvline[n] = 0;
                        process_result = process_remote_bank_message(atm, recvline, n, recvcommand, &recvarg);

                    } while (!process_result);
                    

                    if (atm->session_state == LOGIN_REQ_WAITING) {
                        if(!strcmp(recvcommand, "user-found")) {
                        
                            printf("PIN? ");
                            // take in user input, including newline
                            fgets(pin, DATASIZE + 1, stdin);
                            
                            if (pin[PIN_LEN] != '\n') {
                                while (getchar() != '\n'); // clears rest of stdin
                            }
                            
                            // DEBUG: printf("Checking input pin: %s, length: %d\n", pin, (int)strlen(pin));
                            if(strlen(pin) > (PIN_LEN + 1) || pin[PIN_LEN] != '\n') { // also check with bank records to confirm pin
                                
                                construct_message(atm, to_send, plaintext, ciphertext, "unverifiable", strlen("unverifiable"), -1);
                                atm_send(atm, to_send, strlen(to_send));
                                memset(atm->active_card, 0, CARD_LEN + 1); // clear card
                                atm->session_state = INITIAL;

                                printf("Not authorized\n");
                            } else {
                                
                                char verify_cmd[DATASIZE + 1] = "";

                                strcpy(verify_cmd, "verify ");
                                strncat(verify_cmd, pin, PIN_LEN);
                                verify_cmd[PIN_LEN + 7] = '\0';
                                
                                do { // continues receiving messages until one is accepted
                                    construct_message(atm, to_send, plaintext, ciphertext, verify_cmd, strlen(verify_cmd), -1);
                                    atm_send(atm, to_send, strlen(to_send));
                                    atm->session_state = VERIFY_PIN_WAITING;

                                    // Get bank reply
                                    n = atm_recv(atm, recvline, DATASIZE);
                                    recvline[n] = 0;
                                    process_result = process_remote_bank_message(atm, recvline, n, recvcommand, &recvarg);

                                } while (!process_result);

                                if(atm->session_state == VERIFY_PIN_WAITING) {
                                    if(!strcmp(recvcommand, "access-granted")) {
                                        strncpy(atm->curr_user, user, strlen(user));
                                        atm->curr_user[strlen(user)] = '\0';
                                        atm->session_state = ACTIVE_SESSION;
                                        printf("Authorized\n");
                                    } else { // access-denied case OR any other case
                                        memset(atm->active_card, 0, CARD_LEN + 1); // clear card
                                        atm->session_state = INITIAL;
                                        printf("Not authorized\n");
                                    }
                                }
                                
                            }
                    
                        } else { // user-not-found case OR any other case 
                            memset(atm->active_card, 0, CARD_LEN + 1); // clear card
                            atm->session_state = INITIAL;
                            printf("No such user\n");
                        }  
                    }
                    

                }
            

                memset(user, 0, MAX_USERNAME_LEN + 1); // clear temporary username buffer
                
            }

        } else {
            printf("A user is already logged in\n");
        }

    } else if (strcmp(action, "withdraw") == 0) {
        // DEBUG: printf("WITHDRAW\n");

        if(strlen(atm->curr_user) > 0) {
            // verify amount
            int check_amount = valid_money(arg, &amount);
            
            // check for any non-digit characters
            for(i = 0; i < strlen(arg); i++) {
                if(!isdigit(arg[i])) {
                    invalid = 1; 
                }
            } 
            // check for invalid command input
            if(values != 2 || check_amount == 0 || amount < 0 || invalid) { 
                printf("Usage: withdraw <amt>\n");
            } else {
                char subcommand[DATASIZE + 1] = "";
                
                do { // continues receiving messages until one is accepted
                    strcpy(command_option, "withdraw");
                    construct_message(atm, to_send, plaintext, ciphertext, command_option, strlen(command_option), amount);
                    atm_send(atm, to_send, strlen(to_send));
                    atm->session_state = WITHDRAW_WAITING;

                    // Get bank reply
                    n = atm_recv(atm, recvline, DATASIZE);
                    recvline[n] = 0;
                    process_result = process_remote_bank_message(atm, recvline, n, recvcommand, &recvarg);

                } while (!process_result);
                
                memset(extra, 0, DATASIZE + 1);
                values = sscanf(recvcommand, "%s %s", subcommand, extra);

                if(atm->session_state == WITHDRAW_WAITING) {
                    
                    if(!strcmp(subcommand, "dispense")) {
                        int num = 0;
                        strcpy(command_option, "dispensed");
                        memcpy(&num, recvcommand + strlen(subcommand) + 1, sizeof(int)); // extract amount to dispense
                        construct_message(atm, to_send, plaintext, ciphertext, command_option, strlen(command_option), num);
                        atm_send(atm, to_send, strlen(to_send));
                        atm->session_state = ACTIVE_SESSION;
                        printf("$%d dispensed\n", num);
                    } else { // insufficient case OR any other case
                        atm->session_state = ACTIVE_SESSION;
                        printf("Insufficient funds\n");
                    }
                }

            }
        } else {
            printf("No user logged in\n");
        }

    } else if (strcmp(action, "balance") == 0) { 
        // DEBUG: printf("BALANCE\n");

        if(strlen(atm->curr_user) > 0) {
            if(values == 1) {
                
                char subcommand[DATASIZE + 1] = "";

                do { // continues receiving messages until one is accepted
                    construct_message(atm, to_send, plaintext, ciphertext, "balance", strlen("balance"), -1);
                    atm_send(atm, to_send, strlen(to_send));
                    atm->session_state = BALANCE_WAITING;

                    // Get bank reply
                    n = atm_recv(atm, recvline, DATASIZE);
                    recvline[n] = 0;
                    process_result = process_remote_bank_message(atm, recvline, n, recvcommand, &recvarg);

                } while (!process_result);

                memset(extra, 0, DATASIZE + 1);
                values = sscanf(recvcommand, "%s %s", subcommand, extra);

                if(atm->session_state == BALANCE_WAITING) {
                    
                    if(!strcmp(subcommand, "balance")) {
                        int num = 0;
                        memcpy(&num, recvcommand + strlen(subcommand) + 1, sizeof(int)); // extract balance received
                        atm->session_state = ACTIVE_SESSION;
                        printf("$%d\n", num);
                    } else { // any other case
                        atm->session_state = ACTIVE_SESSION;
                    }   
                }


            } else {
                printf("Usage: balance\n");
            }
            
        } else {
            printf("No user logged in\n");
        }

    } else if (values == 1 && strcmp(action, "end-session") == 0) {
        // DEBUG: printf("END SESSION\n");
        
        if(strlen(atm->curr_user) > 0) {
            end_session(atm);
            printf("User logged out\n");
        } else {
            printf("No user logged in\n");
        }

    } else {
        printf("Invalid command\n");
    }

    printf("\n");

}


// Constructs plaintext from required components, encrypts, and adds IV 
// Places result in to_send
void construct_message(ATM *atm, char *to_send, unsigned char *plaintext, unsigned char *ciphertext, char *cmd_option, int option_len, int arg) {

    struct timeval curr_time;
    
    unsigned char iv[IV_LEN];
    int cipher_len, plain_len;

    gettimeofday(&curr_time, NULL);
    atm->last_mssg_time = curr_time.tv_sec;
    atm->last_mssg_micro_time = curr_time.tv_usec;

    memset(plaintext, 0, DATASIZE + 1);

    // write time stamps 
    memcpy(plaintext, &(curr_time.tv_sec), sizeof(long));
    memcpy(plaintext + sizeof(long), &(curr_time.tv_usec), sizeof(int));
    plaintext += sizeof(long) + sizeof(int);

    // write card number
    memcpy(plaintext, atm->active_card, CARD_LEN);
    plaintext += CARD_LEN;

    // case: command does not contain int 
    if (arg == -1) {
        memcpy(plaintext, cmd_option, option_len);
    } else { // case: command contains int (only active session commands)
        memcpy(plaintext, cmd_option, option_len);
        memcpy(plaintext + option_len + 1, &arg, sizeof(int)); // write int into buffer
    }

    // plaintext now points to beginning of stored cmd_option
    plaintext[option_len] = '\0';

    // encrypt and add IV
    do {
        if(RAND_bytes(iv, IV_LEN) <= 0) {
            continue;
        }
        plain_len = sizeof(long) + sizeof(int) + CARD_LEN + option_len + 1;
        if (arg != -1) {
            plain_len += 4;
        }
        cipher_len = encrypt_aes256_cbc(plaintext, plain_len, atm->key, iv, ciphertext);

        if (cipher_len == 0) {
            continue;
        } else {
            memcpy(to_send, iv, IV_LEN);
            memcpy(to_send + IV_LEN, ciphertext, cipher_len);
            return;
        }
    } while(1);

}

// Takes ciphertext and restores plaintext, returning received command in recvcommand
// and returning integer value 0 if message is rejected, otherwise 1
int process_remote_bank_message(ATM *atm, char *recvline, size_t len, char *recvcommand, int *recvarg) {

    unsigned char iv[IV_LEN], plaintext[DATASIZE + 1];
    char *plain_ptr = (char*) plaintext, curr_card[CARD_LEN + 1];
    int plain_len, micro_msg_time;
    long msg_time;

    // extract IV
    memcpy(iv, recvline, IV_LEN);
    recvline += IV_LEN;

    // decrypt
    plain_len = decrypt_aes256_cbc(recvline, len - IV_LEN, atm->key, iv, plaintext);
    if (plain_len == 0) {
        return 0; // Reject message 
    }

    // extract time stamps 
    memcpy(&msg_time, plain_ptr, sizeof(long));
    memcpy(&micro_msg_time, plain_ptr + sizeof(long), sizeof(int)); // MICROSECONDS CASE 
    plain_ptr += sizeof(long) + sizeof(int);
    plain_len -= sizeof(long) + sizeof(int);

    // check time stamps 
    if (msg_time < atm->last_mssg_time) {
        return 0; // Reject message 
    } else if (msg_time == atm->last_mssg_time && micro_msg_time <= atm->last_mssg_micro_time) { // AND MICROSECONDS CASE
        return 0; // Reject message 
    }
    
    // check card number matches 
    strncpy(curr_card, plain_ptr, CARD_LEN);
    curr_card[CARD_LEN] = '\0';
    if (strcmp(curr_card, atm->active_card)) {
        return 0; // Reject message
    }
    plain_ptr += CARD_LEN;
    plain_len -= CARD_LEN;
    
    // extract command, return in recvcommand
    memcpy(recvcommand, plain_ptr, plain_len);
    recvcommand[plain_len] = '\0';

    return 1;
}

void end_session(ATM *atm) {
    memset(atm->curr_user, 0, MAX_USERNAME_LEN + 1);
    memset(atm->active_card, 0, CARD_LEN + 1);
    atm->session_state = 0;
}