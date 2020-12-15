#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "intmul.h"
#include <errno.h>
#include <string.h>

char *pgmName;
char *argument1;
char *argument2;


void exitError(char *message, int errnum);
void readInput(void);
void cleanUp(void);

int main(int argc, char *argv[])
{
    pgmName = argv[0];
    readInput();
}

void readInput(void)
{
    if ((argument1 = malloc(MAXARGSIZE)) == NULL){
        exitError("Malloc for argument1 failed", errno);
    }

    if ((argument2 = malloc(MAXARGSIZE)) == NULL){
        exitError("Malloc for argument1 failed", errno);
    }

    if (fgets(argument1, MAXARGSIZE, stdin) == NULL)
    {
        exitError("First argument could not be read.", 0);
    }

    if (fgets(argument2, MAXARGSIZE, stdin) == NULL)
    {
        exitError("Second argument could not be read.",0);
    }

    argument1[strlen(argument1)-1] = '\0';
    argument2[strlen(argument2)] = '\0';



    if ((realloc(argument1,strlen(argument1))== NULL))
    {
        exitError("Realloc for argument1 failed.", 0);
    }

    if ((realloc(argument2,strlen(argument2))== NULL))
    {
        exitError("Realloc for argument1 failed.", 0);
    }

    if (strlen(argument1) != strlen(argument2))
    {
        exitError("The arguments are not of equal size.", 0);
    }


    if (strlen(argument1) % 2 != 0 && strlen(argument1) != 1)
    {
        exitError("The given arguments have an odd number of elements.", 0);
    }

    printf("%s\n", argument1);
    printf("%s\n", argument2);
    
}


void cleanUp(void){
    free(argument1);
    free(argument2);
}

void exitError(char *message, int errnum)
{
    cleanUp();
    if (errnum != 0)
    {
        fprintf(stderr, "[%s]: %s: %s\n",pgmName, message, strerror(errnum));
    }else
    {
       fprintf(stderr, "[%s]: %s \n",pgmName, message);
    }
    exit(EXIT_FAILURE);
}