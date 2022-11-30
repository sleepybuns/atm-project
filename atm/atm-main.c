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
    char user_input[DATASIZE + 1] = "";
    FILE *input;
    unsigned char key[KEY_LEN + 1] = "";

    // Check command line argument
    // DEBUG: printf(".atm file: %s\n", argv[1]);
    if((input = fopen(argv[1], "r")) == NULL) {
        printf("Error opening ATM initialization file\n");
        return 64;
    }
    fread(key, 1, KEY_LEN, input);
    // DEBUG: printf("%s\n", key);
    fclose(input);

    ATM *atm = atm_create(key);

    // Check command line argument
    // DEBUG: printf(".atm file: %s\n", argv[1]);
    if(fopen(argv[1], "r") == NULL) {
        printf("Error opening ATM initialization file\n");
        return 64;
    }

    printf("%s", prompt);
    fflush(stdout);

    while (fgets(user_input, DATASIZE + 1, stdin) != NULL)
    {
        if (user_input[strlen(user_input) - 1] != '\n') {
            while (getchar() != '\n'); // clears rest of stdin
        }

        atm_process_command(atm, user_input);
        if(strlen(atm->curr_user) > 0) {
            printf("ATM (%s): ", atm->curr_user);
        } else {
            printf("%s", prompt);
        }
        
        fflush(stdout);
    }
	return EXIT_SUCCESS;
}
