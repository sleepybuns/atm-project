#include "atm.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <openssl/rand.h>

ATM* atm_create(char *data)
{
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

    // Set up the protocol state
    // TODO set up more, as needed

    // No session initially
    memset(atm->curr_user, 0, MAX_USERNAME_LEN + 1);
    atm->session_state = 0;

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

    char recvline[DATASIZE + 1] = "", action[DATASIZE + 1] = "", 
    arg[DATASIZE + 1] = "", extra[DATASIZE + 1] = "", pin[DATASIZE + 1] = "";
    char user[MAX_USERNAME_LEN + 1] = "";
    unsigned char iv[IV_LEN];
    int n, values = 0, invalid = 0, i, amount;

    // Generate IV, returns 1 on success
    if((RAND_bytes(iv, sizeof(iv))) == 0) {
        fputs("Failed to generate IV\n", stderr);
    }
    
    /*
	 * The following is a toy example that simply sends the
	 * user's command to the bank, receives a message from the
	 * bank, and then prints it to stdout.
	 */

    /*
    atm_send(atm, command, strlen(command));
    n = atm_recv(atm,recvline,10000);
    recvline[n]=0;
    fputs(recvline,stdout);
	*/

    // for debugging 
    // printf(" String read in: %s\n", command);

    values = sscanf(command, "%s %s %s", action, arg, extra);

    if(strcmp(action, "begin-session") == 0) {
        // DEBUG: printf("BEGIN SESSION\n");

        if(strlen(atm->curr_user) == 0) { // checks for active session

            int invalid = 0, i; 

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

                // TODO: Check is user exists in bank system
                // print "No such user" if not

                // save username
                strncpy(user, arg, strlen(arg));
                user[strlen(arg)] = '\0';
                // DEBUG: printf("Checking user: %s\n", user);
                
                // check <user>.card file is present
                strcat(arg, ".card");
                if(fopen(arg, "r") != NULL) { // looks in curr dir (bin)

                    printf("PIN? ");
                    // take in user input, including newline
                    fgets(pin, 20, stdin);
                    
                    // DEBUG: printf("Checking input pin: %s, length: %d\n", pin, (int)strlen(pin));
                    if(strlen(pin) > (PIN_LEN + 1) || pin[PIN_LEN] != '\n') { // also check with bank records to confirm pin
                        printf("Not authorized\n");
                    } else {
                        printf("Authorized\n");
                        strncpy(atm->curr_user, user, strlen(user));
                        atm->curr_user[strlen(user)] = '\0';
                    }
        
                } else {
                    printf("Unable to access %s's card\n", user);
                }
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

                // TODO: Implement interaction with bank
                /*
                atm_send(atm, command, strlen(command));
                n = atm_recv(atm, recvline, 10000);
                recvline[n] = 0;
                fputs(recvline, stdout);
                */

            }
        } else {
            printf("No user logged in\n");
        }

    } else if (strcmp(action, "balance") == 0) { 
        // DEBUG: printf("BALANCE\n");

        if(strlen(atm->curr_user) > 0) {
            if(values == 1) {

                // TODO: Implement interaction with bank to get and print balance
                /*
                atm_send(atm, command, strlen(command));
                n = atm_recv(atm, recvline, 10000);
                recvline[n] = 0;
                fputs(recvline, stdout);
                */

            } else {
                printf("Usage: balance\n");
            }
            
        } else {
            printf("No user logged in\n");
        }

    } else if (values == 1 && strcmp(action, "end-session") == 0) {
        // DEBUG: printf("END SESSION\n");
        
        if(strlen(atm->curr_user) > 0) {
            memset(atm->curr_user, 0, MAX_USERNAME_LEN + 1);
            printf("User logged out\n");
        } else {
            printf("No user logged in\n");
        }

    } else {
        printf("Invalid command\n");
    }

    printf("\n");

}
