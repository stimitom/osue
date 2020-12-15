#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "intmul.h"
#include <errno.h>
#include <string.h>
#include <ctype.h>

char *pgmName;
char *argument1;
char *argument2;
char *result;
char *Ah;
char *Al;
char *Bh;
char *Bl;
pid_t pids[4];

void exitError(char *message, int errnum);
void readInput(void);
void cleanUp(void);
void argumentsAreValidHexa(void);
void multiplyAndWrite(void);
void splitArguments(int numberOfDigits);
void forkIntoChildProcesses(void);
void createPipes(void);
void openPipes(void); 
void fillPipes(void);
void closeUnneccessaryPipes(void);

int main(int argc, char *argv[])
{
    pgmName = argv[0];

    readInput();
    int numberOfDigits = strlen(argument1);

    if (numberOfDigits == 1)
    {
        multiplyAndWrite();
        exit(EXIT_SUCCESS);
    }

    splitArguments(numberOfDigits);

    // int pipefd[2];
    // if (pipe(pipefd) == -1)
    // {
    //     exitError("Pipe could not be created.", errno);
    // }

    for (int i = 0; i < 4; i++)
    {
        switch (pids[i])
        {
        case -2:
            //not set
            break;
        case 0:
            //child
            break;
        default:
            //parent of pids[i]
            break;
        }
    }
}

void createPipes(void)
{
}

void forkIntoChildProcesses(void)
{
    pid_t pidAh = -2;
    pid_t pidAl = -2;
    pid_t pidBh = -2;
    pid_t pidBl = -2;

    if ((pidAh = fork()) == -1)
    {
        exitError("Could not fork child process Ah", errno);
    }

    if ((pidAl = fork()) == -1)
    {
        exitError("Could not fork child process Al", errno);
    }

    if ((pidBh = fork()) == -1)
    {
        exitError("Could not fork child process Bh", errno);
    }

    if ((pidBl = fork()) == -1)
    {
        exitError("Could not fork child process Bl", errno);
    }
    pids[1] = pidAh;
    pids[2] = pidAl;
    pids[3] = pidBh;
    pids[4] = pidBl;
}

void splitArguments(int numberOfDigits)
{
    if ((Ah = malloc(numberOfDigits * sizeof(char))) == NULL)
    {
        exitError("Malloc for Ah failed", errno);
    }

    if ((Al = malloc(numberOfDigits * sizeof(char))) == NULL)
    {
        exitError("Malloc for Al failed", errno);
    }

    if ((Bh = malloc(numberOfDigits * sizeof(char))) == NULL)
    {
        exitError("Malloc for Bh failed", errno);
    }

    if ((Bl = malloc(numberOfDigits * sizeof(char))) == NULL)
    {
        exitError("Malloc for Bl failed", errno);
    }

    for (int i = 0; i < numberOfDigits / 2; i++)
    {
        Ah[i] = argument1[i];
        Al[i] = argument1[i + (numberOfDigits / 2)];
        Bh[i] = argument2[i];
        Bl[i] = argument2[i + (numberOfDigits / 2)];
    }

    Ah[numberOfDigits / 2] = '\0';
    Al[numberOfDigits / 2] = '\0';
    Bh[numberOfDigits / 2] = '\0';
    Bl[numberOfDigits / 2] = '\0';
}

void multiplyAndWrite(void)
{
    long int aHex;
    char *endpointer;
    aHex = strtol(argument1, &endpointer, 16);

    if (endpointer != NULL)
    {
        if (endpointer == argument1)
        {
            exitError("Argument1 could not be converted to a long int.", 0);
        }
    }

    long int bHex;
    char *endpointer2;
    bHex = strtol(argument2, &endpointer2, 16);

    if (endpointer2 != NULL)
    {
        if (endpointer2 == argument2)
        {
            exitError("Argument2 could not be converted to a long int.", 0);
        }
    }

    long int res = aHex * bHex;

    if ((result = malloc(sizeof(char) * 2)) == NULL)
    {
        exitError("Malloc for result failed", errno);
    }

    fprintf(stdout, "%x", (int)res);
}

void readInput(void)
{
    if ((argument1 = malloc(MAXARGSIZE)) == NULL)
    {
        exitError("Malloc for argument1 failed", errno);
    }

    if ((argument2 = malloc(MAXARGSIZE)) == NULL)
    {
        exitError("Malloc for argument1 failed", errno);
    }

    if (fgets(argument1, MAXARGSIZE, stdin) == NULL)
    {
        exitError("First argument could not be read.", 0);
    }

    if (fgets(argument2, MAXARGSIZE, stdin) == NULL)
    {
        exitError("Second argument could not be read.", 0);
    }

    argument1[strlen(argument1) - 1] = '\0';
    argument2[strlen(argument2)] = '\0';

    if ((realloc(argument1, strlen(argument1)) == NULL))
    {
        exitError("Realloc for argument1 failed.", 0);
    }

    if ((realloc(argument2, strlen(argument2)) == NULL))
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

    argumentsAreValidHexa();
}

void argumentsAreValidHexa(void)
{
    for (int i = 0; i < strlen(argument1); i++)
    {
        if (!isxdigit(argument1[i]))
        {
            exitError("Argument1 is not a valid hex number.", 0);
        }
        if (!isxdigit(argument2[i]))
        {
            exitError("Argument2 is not a valid hex number.", 0);
        }
    }
}

void cleanUp(void)
{
    free(argument1);
    free(argument2);
}

void exitError(char *message, int errnum)
{
    cleanUp();
    if (errnum != 0)
    {
        fprintf(stderr, "[%s]: %s: %s\n", pgmName, message, strerror(errnum));
    }
    else
    {
        fprintf(stderr, "[%s]: %s \n", pgmName, message);
    }
    exit(EXIT_FAILURE);
}