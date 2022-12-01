/*
 * The Bank takes commands from stdin as well as from the ATM.  
 *
 * Commands from stdin be handled by bank_process_local_command.
 *
 * Remote commands from the ATM should be handled by
 * bank_process_remote_command.
 *
 * The Bank can read both .card files AND .pin files.
 *
 * Feel free to update the struct and the processing as you desire
 * (though you probably won't need/want to change send/recv).
 */

#ifndef __BANK_H__
#define __BANK_H__

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/time.h>
#include "../util/hash_table.h"

#define DATASIZE 1000
#define MAX_USERNAME_LEN 250
#define PIN_LEN 4
#define KEY_LEN 32
#define IV_LEN 16
#define HASH_LEN 32
#define CARD_LEN 32
//protocol states
#define NO_SESH 0
#define AWAIT_PIN 99
#define OPEN_SESH 11
#define WITHDRAW 33


typedef struct userfile {
    int balance;
    char pin[PIN_LEN + 1];
    char card_num[CARD_LEN + 1];
} Userfile;

typedef struct _Bank {
    // Networking state
    int sockfd;
    struct sockaddr_in rtr_addr;
    struct sockaddr_in bank_addr;
    unsigned char key[KEY_LEN];
    // Protocol state
    // TODO add more, as needed
    HashTable *accounts;
    HashTable *acc_nums;
    
    Userfile *logged_user;
    char active_card[CARD_LEN + 1];
    char active_user[MAX_USERNAME_LEN + 1];
    int session_state;
    time_t last_msg_sec;
    suseconds_t last_msg_usec; 


} Bank;


Bank* bank_create();
void bank_free(Bank *bank);
ssize_t bank_send(Bank *bank, unsigned char *data, size_t data_len);
ssize_t bank_recv(Bank *bank, unsigned char *data, size_t max_data_len);
void bank_process_local_command(Bank *bank, char *command, size_t len);
void bank_process_remote_command(Bank *bank, unsigned char *command, size_t len);
void gen_card_num(Bank *bank, char *card_num, unsigned char *plaintext, int plaintext_len);
int construct_response (Bank *bank, unsigned char *response, char *command, int cmd_len);
int construct_response_initial (Bank *bank, unsigned char *response, char *command, int cmd_len);
#endif

