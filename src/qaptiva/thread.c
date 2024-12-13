#include<stdio.h>
#include <string.h>

int main() {
   char string[50] = "Hello! We are learning about strtok";
   // Extract the first token
   char * token = strtok(string, " ");
   // loop through the string to extract all other tokens
   while( token != NULL ) {
      printf( " %s\n", token ); //printing each token
      token = strtok(NULL, " ");
   }

   printf("%s\n", token);
   return 0;
}