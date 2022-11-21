/* 
 * The main program for the ATM.
 *
 * You are free to change this as necessary.
 */

#include "atm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char prompt[] = "ATM: ";

int main(int argc, char *argv[])
{
    char user_input[1000];

    ATM *atm = atm_create();

    // Check command line argument
    // DEBUG: printf(".atm file: %s\n", argv[1]);
    if(fopen(argv[1], "r") == NULL) {
        printf("Error opening ATM initialization file\n");
        return 64;
    }

    printf("%s", prompt);
    fflush(stdout);

    // No session initially
    curr_user[0] = '\0';

    while (fgets(user_input, 10000,stdin) != NULL)
    {
        atm_process_command(atm, user_input);
        if(strlen(curr_user) > 0) {
            printf("ATM (%s): ", curr_user);
        } else {
            printf("%s", prompt);
        }
        
        fflush(stdout);
    }
	return EXIT_SUCCESS;
}
