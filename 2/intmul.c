#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "intmul.h"
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

char *pgmName;
char *argument1;
char *argument2;
char *result;
char *Ah;
char *Al;
char *Bh;
char *Bl;
pid_t pids[4];
int pipefds[8][2];

static void exitError(char *message, int errnum);
static void readInput(void);
static void cleanUp(void);
static void argumentsAreValidHexa(void);
static void multiplyAndWrite(void);
static void splitArguments(int numberOfDigits);
static void forkIntoChildProcesses(void);
static void createPipes(void);
static void closeUnneccasaryPipeEndsParent(void);
static void redirectInAndOut(void);
static void closeAllPipesExcept(int readPipeNumber, int writePipeNumber);
static void writeToPipes(void);
static void waitForChildren(void);

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

    createPipes();

    forkIntoChildProcesses();

    redirectInAndOut();

    if (pids[0] && pids[1] && pids[2] && pids[3])
    {
        //Only here in parent
        closeUnneccasaryPipeEndsParent();
        writeToPipes();
        waitForChildren();
    }
}

static void waitForChildren(void)
{   
    int statuses[4];
    if (waitpid(pids[0], &statuses[0], 0) == -1)
    {
        exitError("Waiting for highest child failed.", errno);
    }
    if (waitpid(pids[1], &statuses[1], 0) == -1)
    {
        exitError("Waiting for highest child failed.", errno);
    }
    if (waitpid(pids[2], &statuses[2], 0) == -1)
    {
        exitError("Waiting for highest child failed.", errno);
    }
    if (waitpid(pids[3], &statuses[3], 0) == -1)
    {
        exitError("Waiting for highest child failed.", errno);
    }

    if (!WIFEXITED(statuses[0]))
    {
        exitError(" A Child process did not terminate properly. Status", WEXITSTATUS(statuses[0]));
    }

    if (!WIFEXITED(statuses[1]))
    {
        exitError(" A Child process did not terminate properly. Status", WEXITSTATUS(statuses[1]));
    }

    if (!WIFEXITED(statuses[2]))
    {
        exitError(" A Child process did not terminate properly. Status", WEXITSTATUS(statuses[2]));
    }

    if (!WIFEXITED(statuses[3]))
    {
        exitError(" A Child process did not terminate properly. Status", WEXITSTATUS(statuses[3]));
    }
}

static void writeToPipes(void)
{
    //Highest
    if (dprintf(pipefds[0][1], Ah) <=  0)
    {
        exitError("Could not write Ah to readPipe in Parent.", errno);
    }
    if ((write(pipefds[0][1], Bh, strlen(Bh)) == -1))
    {
        exitError("Could not write Bh to readPipe in Parent.", errno);
    }
    if (close(pipefds[0][1]) == -1)
    {
        exitError("Pipe end could not be closed.", errno);
    }

    // Highlow
    if ((write(pipefds[2][1], Ah, strlen(Ah)) == -1))
    {
        exitError("Could not write Ah to readPipe in Parent.", errno);
    }
    if ((write(pipefds[2][1], Bl, strlen(Bl)) == -1))
    {
        exitError("Could not write Bh to readPipe in Parent.", errno);
    }
    if (close(pipefds[2][1]) == -1)
    {
        exitError("Pipe end could not be closed.", errno);
    }

    // LowHigh
    if ((write(pipefds[4][1], Al, strlen(Al)) == -1))
    {
        exitError("Could not write Ah to readPipe in Parent.", errno);
    }
    if ((write(pipefds[4][1], Bh, strlen(Bh)) == -1))
    {
        exitError("Could not write Bh to readPipe in Parent.", errno);
    }
    if (close(pipefds[4][1]) == -1)
    {
        exitError("Pipe end could not be closed.", errno);
    }

    // Lowest
    if ((write(pipefds[6][1], Al, strlen(Al)) == -1))
    {
        exitError("Could not write Ah to readPipe in Parent.", errno);
    }
    if ((write(pipefds[6][1], Bl, strlen(Bl)) == -1))
    {
        exitError("Could not write Bh to readPipe in Parent.", errno);
    }
    if (close(pipefds[6][1]) == -1)
    {
        exitError("Pipe end could not be closed.", errno);
    }
}

static void closeAllPipesExcept(int readPipeNumber, int writePipeNumber)
{
    for (int i = 0; i < 8; i++)
    {
        if (i != readPipeNumber)
        {
            if (close(pipefds[i][0]) == -1)
            {
                exitError("Pipe read end could not be closed.", errno);
            }
        }
        if (i != writePipeNumber)
        {
            if (close(pipefds[i][1]) == -1)
            {
                exitError("Pipe write end could not be closed.", errno);
            }
        }
    }
}

static void redirectInAndOut(void)
{
    if (!pids[0])
    {
        //Child process highest
        if (dup2(pipefds[0][0], STDIN_FILENO) == -1)
        {
            exitError("Could not redirect stdin to readpipe.", errno);
        }

        if (dup2(pipefds[1][1], STDOUT_FILENO) == -1)
        {
            exitError("Could not redirect stdout to writepipe.", errno);
        }

        closeAllPipesExcept(0, 1);
    }
    else if (!pids[1])
    {
        //Child process highlow
        if (dup2(pipefds[2][0], STDIN_FILENO) == -1)
        {
            exitError("Could not redirect stdin to readpipe.", errno);
        }

        if (dup2(pipefds[3][1], STDOUT_FILENO) == -1)
        {
            exitError("Could not redirect stdout to writepipe.", errno);
        }
        closeAllPipesExcept(2, 3);
    }
    else if (!pids[2])
    {
        //child process lowhigh
        if (dup2(pipefds[4][0], STDIN_FILENO) == -1)
        {
            exitError("Could not redirect stdin to readpipe.", errno);
        }

        if (dup2(pipefds[5][1], STDOUT_FILENO) == -1)
        {
            exitError("Could not redirect stdout to writepipe.", errno);
        }

        closeAllPipesExcept(4, 5);
    }
    else if (!pids[3])
    {
        //child process lowest
        if (dup2(pipefds[6][0], STDIN_FILENO) == -1)
        {
            exitError("Could not redirect stdin to readpipe.", errno);
        }

        if (dup2(pipefds[7][1], STDOUT_FILENO) == -1)
        {
            exitError("Could not redirect stdout to writepipe.", errno);
        }

        closeAllPipesExcept(6, 7);
    }else{
        //parent 
        return;
    }
    execlp
}

static void createPipes(void)
{
    for (int i = 0; i < 8; i++)
    {
        if (pipe(pipefds[i]) == -1)
        {
            exitError("Pipe could not be created.", errno);
        }
    }
}

static void closeUnneccasaryPipeEndsParent(void)
{
    for (int i = 0; i < 8; i++)
    {
        if ((i % 2) == 0)
        {
            // Closes Read Pipe Read End From Parent
            if (close(pipefds[i][0]) == -1)
            {
                exitError("Pipe end could not be closed.", errno);
            }
        }
        else
        {
            // Closes Write Pipe Write End From Parent
            if (close(pipefds[i][1]) == -1)
            {
                exitError("Pipe end could not be closed.", errno);
            }
        }
    }
}

static void forkIntoChildProcesses(void)
{
    pid_t pidHighest = -2;
    pid_t pidHighLow = -2;
    pid_t pidLowHigh = -2;
    pid_t pidLowest = -2;

    if ((pidHighest = fork()) == -1)
    {
        exitError("Could not fork child process Ah.", errno);
    }

    if (pidHighest > 0)
    {
        if ((pidHighLow = fork()) == -1)
        {
            exitError("Could not fork child process Al.", errno);
        }
    }
    if (pidHighLow > 0)
    {
        if ((pidLowHigh = fork()) == -1)
        {
            exitError("Could not fork child process Bh.", errno);
        }
    }
    if (pidLowHigh > 0)
    {
        if ((pidLowest = fork()) == -1)
        {
            exitError("Could not fork child process Bl.", errno);
        }
    }

    pids[0] = pidHighest;
    pids[1] = pidHighLow;
    pids[2] = pidLowHigh;
    pids[3] = pidLowest;
}

static void splitArguments(int numberOfDigits)
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

static void multiplyAndWrite(void)
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

static void readInput(void)
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

static void argumentsAreValidHexa(void)
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

static void cleanUp(void)
{
    free(argument1);
    free(argument2);
}

static void exitError(char *message, int errnum)
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