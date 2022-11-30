/*
 * The ATM interfaces with the user.  User commands should be
 * handled by atm_process_command.
 *
 * The ATM can read .card files, but not .pin files.
 *
 * Feel free to update the struct and the processing as you desire
 * (though you probably won't need/want to change send/recv).
 */

#ifndef __ATM_H__
#define __ATM_H__

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/time.h>
#include "../util/misc_util.h"
#define DATASIZE 1000
#define MAX_USERNAME_LEN 250
#define KEY_LEN 32
#define IV_LEN 16
#define PIN_LEN 4
#define CARD_LEN 32

#define INITIAL 0 
#define LOGIN_REQ_WAITING 11
#define VERIFY_PIN_WAITING 22
#define ACTIVE_SESSION 33
#define WITHDRAW_WAITING 44
#define BALANCE_WAITING 55


char curr_user[20]; // Indicator of an active session

typedef struct _ATM
{
    // Networking state
    int sockfd;
    struct sockaddr_in rtr_addr;
    struct sockaddr_in atm_addr;

    // Protocol state
    // TODO add more, as needed
    char curr_user[MAX_USERNAME_LEN + 1];
    int session_state;
    unsigned char key[KEY_LEN];
    char active_card[CARD_LEN + 1];
    time_t last_mssg_time; 
    suseconds_t last_mssg_micro_time;

} ATM;

ATM* atm_create();
void atm_free(ATM *atm);
ssize_t atm_send(ATM *atm, unsigned char *data, size_t data_len);
ssize_t atm_recv(ATM *atm, unsigned char *data, size_t max_data_len);
void atm_process_command(ATM *atm, char *command);
void construct_message(ATM *atm, unsigned char *to_send, unsigned char *plaintext, unsigned char *ciphertext, char *cmd_option, int option_len, int arg);
int process_remote_bank_message(ATM *atm, unsigned char *recvline, size_t len, char *recvcommand, int *recvarg);
void end_session(ATM *atm);
#endif
