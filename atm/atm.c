#include "atm.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

ATM* atm_create()
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

    char recvline[10000], action[20], arg[256], extra[20], pin[20];
    char user[251];
    int n, values = 0, amount;

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

        if(strlen(curr_user) == 0) { // checks for active session

            int invalid = 0, i; 

            // check for any non-alphabetic characters
            for(i = 0; i < strlen(arg);  i++) {
                if(!isalpha(arg[i])) {
                    invalid = 1; 
                }
            } 
            // check for invalid command input
            if(values != 2 || strlen(arg) > 250 || invalid) {
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
                    // take in user input
                    fgets(pin, 20, stdin);
                    
                    // DEBUG: printf("Checking input pin: %s, length: %d\n", pin, (int)strlen(pin));
                    if(strlen(pin) > 5 || pin[4] != '\n') { // also check with bank records to confirm pin
                        printf("Not authorized\n");
                    } else {
                        printf("Authorized\n");
                        strncpy(curr_user, user, strlen(user));
                        curr_user[strlen(user)] = '\0';
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
        
        if(strlen(curr_user) > 0) {
            // verify amount
            amount = (int) strtol(arg, (char **)NULL, 10); // used instead of atoi for error handling purposes

            // DEBUG: printf("testing if strtol worked, val: %d\n", amount);

            // check for invalid command input
            if(values != 2 || amount <= 0) { // assuming that 'withdraw 0' is invalid ??
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

        if(strlen(curr_user) > 0) {
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
        
        if(strlen(curr_user) > 0) {
            curr_user[0] = '\0';
            printf("User logged out\n");
        } else {
            printf("No user logged in\n");
        }

    } else {
        printf("Invalid command\n");
    }

    printf("\n");

}
