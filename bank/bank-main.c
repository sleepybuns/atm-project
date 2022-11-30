/* 
 * The main program for the Bank.
 *
 * You are free to change this as necessary.
 */

#include <string.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include "bank.h"
#include "ports.h"


static const char prompt[] = "BANK: ";

int main(int argc, char**argv) {
   int n;
   char sendline[DATASIZE + 1] = "";
   unsigned char recvline[DATASIZE + 1] = "";
   FILE *input;
   unsigned char key[KEY_LEN + 1]  = "";

   if ((input = fopen(argv[1], "r")) == NULL) {
      printf("Error opening bank initialization file\n");
      return 64;
   } 
   fread(key, 1, KEY_LEN, input);
   fclose(input);

   Bank *bank = bank_create(key);

   printf("%s", prompt);
   fflush(stdout);

   while(1) {
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(0, &fds);
      FD_SET(bank->sockfd, &fds);
      select(bank->sockfd+1, &fds, NULL, NULL, NULL);

      if(FD_ISSET(0, &fds)) {
         fgets(sendline, DATASIZE + 1, stdin);
         
         if (sendline[strlen(sendline) - 1] != '\n') {
            while (getchar() != '\n'); //clear rest of stdin
         }
         bank_process_local_command(bank, sendline, strlen(sendline));
         printf("%s", prompt);
         fflush(stdout);
      } else if(FD_ISSET(bank->sockfd, &fds)) {
         n = bank_recv(bank, recvline, DATASIZE);
         bank_process_remote_command(bank, recvline, n);
      }
   }

   return EXIT_SUCCESS;
}
