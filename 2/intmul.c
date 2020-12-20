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
pid_t pids[4];
int pipefds[8][2];
char *highestVal = NULL;
char *highLowVal = NULL;
char *lowHighVal = NULL;
char *lowestVal = NULL;

static void exitError(char *message, int errnum);
static void readInput(void);
static void cleanUp(void);
static void argumentsAreValidHexa(void);
static void multiplyAndWrite(void);
static void forkIntoChildProcesses(void);
static void createPipes(void);
static void closeUnneccasaryPipeEndsParent(void);
static void redirectInAndOut(void);
static void closeAllPipesExcept(int readPipeNumber, int writePipeNumber);
static void writeToPipes(char *Ah, char *Bh, char *Al, char *Bl);
static void waitForChildren(void);
static void executeChildProcess(void);
static void readFromChildren(int numberOfDigits);
static void appendZeroes(char **initial, int numberOfDigits);
static void addPartSolutions(char *highest, char *highlow, char *lowhigh, char *lowest);
static int hexCharToInt(char character);
static void fromIntToHexChar(int i, char *c);
static char *reverseString(char *str);
static void addHexStrings(char *a, char *b, char **endResult);

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

    int halfNumberOfDigits = numberOfDigits / 2;

    char Ah[halfNumberOfDigits];
    char Al[halfNumberOfDigits];
    char Bh[halfNumberOfDigits];
    char Bl[halfNumberOfDigits];

    for (int i = 0; i <= halfNumberOfDigits; i++)
    {
        Ah[i] = argument1[i];
        Al[i] = argument1[i + halfNumberOfDigits];
        Bh[i] = argument2[i];
        Bl[i] = argument2[i + halfNumberOfDigits];

        if (i == halfNumberOfDigits)
        {
            Ah[i] = '\0';
            Al[i] = '\0';
            Bh[i] = '\0';
            Bl[i] = '\0';
        }
    }

    createPipes();

    forkIntoChildProcesses();

    redirectInAndOut();

    if (pids[0] && pids[1] && pids[2] && pids[3])
    {
        //Parent
        closeUnneccasaryPipeEndsParent();
        writeToPipes(Ah, Bh, Al, Bl);
        waitForChildren();
        readFromChildren(numberOfDigits);
    }
    else
    {
        // Child
        executeChildProcess();
    }
}

static int hexCharToInt(char character)
{

    if (character >= '0' && character <= '9')
    {
        return character - '0';
    }
    if (character >= 'A' && character <= 'F')
    {

        return character - 'A' + 10;
    }
    if (character >= 'a' && character <= 'f')
    {
        return character - 'a' + 10;
    }
    return -1;
}

static void fromIntToHexChar(int i, char *c)
{
    sprintf(c, "%X", i);
}

static char *reverseString(char *str)
{

    if (!str || !*str)
    {
        return str;
    }

    int i = strlen(str) - 1, j = 0;

    char c;
    while (i > j)
    {
        c = str[i];
        str[i] = str[j];
        str[j] = c;
        i--;
        j++;
    }
    return str;
}

static void addHexStrings(char *a, char *b, char **endResult)
{
    char *first;
    char *second;

    // Length of second needs to be > length of first
    if (strlen(a) > strlen(b))
    {
        if ((second = malloc(strlen(a) + 1)) == NULL)
        {
            exitError("Malloc for second failed.", errno);
        }
        strncpy(second, a, strlen(a) + 1);
        if ((first = malloc(strlen(b) + 1)) == NULL)
        {
            exitError("Malloc for first failed.", errno);
        }
        strncpy(first, b, strlen(b) + 1);
    }
    else
    {
        if ((second = malloc(strlen(b) + 1)) == NULL)
        {
            exitError("Malloc for second failed.", errno);
        }
        strncpy(second, b, strlen(b) + 1);
        if ((first = malloc(strlen(a) + 1)) == NULL)
        {
            exitError("Malloc for first failed.", errno);
        }
        strncpy(first, a, strlen(a) + 1);
    }

    int n1 = strlen(first), n2 = strlen(second);
    int diff = n2 - n1;
    int resultLen = 0;

    // Take an empty string for storing result
    char current[(n1 + n2 - 1)];
    bzero(current, (n1 + n2 - 2));
    current[n1 + n2 - 2] = '\0';

    // Initially take carry zero
    char carry[2];
    carry[0] = '0';
    carry[1] = '\0';

    char temp[2];
    temp[0] = '0';
    temp[1] = '\0';

    // Traverse from end of both strings
    for (int i = n1 - 1; i >= 0; i--)
    {
        int sum = hexCharToInt(first[i]) + hexCharToInt(second[i + diff]) + hexCharToInt(carry[0]);
        fromIntToHexChar((sum % 16), &temp[0]);
        strncat(current, temp, 2);
        fromIntToHexChar((sum / 16), &carry[0]);
        resultLen++;
    }

    // Add remaining digits of str2[]
    char temp2[2];
    temp2[0] = '0';
    temp2[1] = '\0';

    for (int i = n2 - n1 - 1; i >= 0; i--)
    {
        int sum = hexCharToInt(second[i]) + hexCharToInt(carry[0]);

        fromIntToHexChar((sum % 16), &temp2[0]);

        strncat(current, temp2, 2);
        fromIntToHexChar((sum / 16), &carry[0]);
        resultLen++;
    }

    // Add remaining carry
    if (carry[0] != '0')
    {
        strncat(current, carry, 2);
        resultLen++;
    }

    void *newpointer;
    if ((newpointer = realloc(*endResult, resultLen+1)))
    {
        *endResult = newpointer;
    }
    else
    {
        exitError("Realloc for endresult failed.", errno);
    }

    free(first);
    free(second);

    strncpy(*endResult, reverseString(current), resultLen+1);
}

static void addPartSolutions(char *highest, char *highlow, char *lowhigh, char *lowest)
{
    char *total;
    if ((total = malloc(1)) == NULL)
    {
        exitError("Malloc for total failed.", errno);
    }
    addHexStrings(highest, highlow, &total);
    addHexStrings(total, lowhigh, &total);
    addHexStrings(total, lowest, &total);
    fprintf(stdout, "%s\n", total);
    free(total);
    cleanUp();
    exit(EXIT_SUCCESS);
}

static void appendZeroes(char **initial, int numberOfDigits)
{
    int oldlength = strlen(*initial);
    void *newpointer;
    if ((newpointer = realloc(*initial, (oldlength + numberOfDigits + 1))))
    {
        *initial = newpointer;
    }
    else
    {
        exitError("Realloc for char 'initial' failed.", errno);
    }

    for (int i = 0; i < numberOfDigits; i++)
    {
        (*initial)[i + oldlength] = '0';
    }
    (*initial)[oldlength + numberOfDigits] = '\0';
}

static void readFromChildren(int numberOfDigits)
{
    FILE *highest;
    FILE *highLow;
    FILE *lowHigh;
    FILE *lowest;

    //Highest
    if ((highest = fdopen(pipefds[1][0], "r")) == NULL)
    {
        exitError("FILE 'highest' could not be opened.", errno);
    }

    size_t lenHighest = 0;
    ssize_t linesizeHighest;
    while ((linesizeHighest = getline(&highestVal, &lenHighest, highest)) != -1)
    {
        if (highestVal[linesizeHighest - 1] == '\n')
        {
            highestVal[linesizeHighest - 1] = '\0';
            --linesizeHighest;
        }
    }

    if (fclose(highest) == EOF)
    {
        exitError("'highest' could not be closed.", errno);
    }

    // Highlow
    if ((highLow = fdopen(pipefds[3][0], "r")) == NULL)
    {
        exitError("FILE 'highLow' could not be opened.", errno);
    }

    size_t lenHighLow = 0;
    ssize_t linesizeHighLow;
    while ((linesizeHighLow = getline(&highLowVal, &lenHighLow, highLow)) != -1)
    {
        if (highLowVal[linesizeHighLow - 1] == '\n')
        {
            highLowVal[linesizeHighLow - 1] = '\0';
            --linesizeHighLow;
        }
    }


    if (fclose(highLow) == EOF)
    {
        exitError("'highlow' could not be closed.", errno);
    }

    //lowhigh
    if ((lowHigh = fdopen(pipefds[5][0], "r")) == NULL)
    {
        exitError("FILE 'lowHigh' could not be opened.", errno);
    }

    size_t lenLowHigh = 0;
    ssize_t linesizeLowHigh;
    while ((linesizeLowHigh = getline(&lowHighVal, &lenLowHigh, lowHigh)) != -1)
    {
        if (lowHighVal[linesizeLowHigh - 1] == '\n')
        {
            lowHighVal[linesizeLowHigh - 1] = '\0';
            --linesizeLowHigh;
        } 
    }

    if (fclose(lowHigh) == EOF)
    {
        exitError("'lowhigh' could not be closed.", errno);
    }

    //Lowest
    if ((lowest = fdopen(pipefds[7][0], "r")) == NULL)
    {
        exitError("FILE 'lowest' could not be opened.", errno);
    }

    size_t lenLowest = 0;
    ssize_t linesizeLowest;
    while ((linesizeLowest = getline(&lowestVal, &lenLowest, lowest)) != -1)
    {
        if (lowestVal[linesizeLowest - 1] == '\n')
        {
            lowestVal[linesizeLowest - 1] = '\0';
            --linesizeLowest;
        }
    }

    if (fclose(lowest) == EOF)
    {
        exitError("'lowest' could not be closed.", errno);
    }

    appendZeroes(&highestVal, numberOfDigits);
    appendZeroes(&highLowVal, numberOfDigits / 2);
    appendZeroes(&lowHighVal, numberOfDigits / 2);
    addPartSolutions(highestVal, highLowVal, lowHighVal, lowestVal);
}

static void executeChildProcess(void)
{
    if (execlp(pgmName, pgmName, (char *)NULL) == -1)
    {
        exitError("The child programm could not be executed.", errno);
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
        fprintf(stdout, "A Child process did not terminate properly. Status %s\n", strerror(WEXITSTATUS(statuses[0])));
        exit(EXIT_FAILURE);
    }

    if (!WIFEXITED(statuses[1]))
    {
        fprintf(stdout, "A Child process did not terminate properly. Status %s\n", strerror(WEXITSTATUS(statuses[1])));
        exit(EXIT_FAILURE);
    }

    if (!WIFEXITED(statuses[2]))
    {
        fprintf(stdout, "A Child process did not terminate properly. Status %s\n", strerror(WEXITSTATUS(statuses[2])));
        exit(EXIT_FAILURE);
    }

    if (!WIFEXITED(statuses[3]))
    {
        fprintf(stdout, "A Child process did not terminate properly. Status %s\n", strerror(WEXITSTATUS(statuses[3])));
        exit(EXIT_FAILURE);
    }
}

static void writeToPipes(char *Ah, char *Bh, char *Al, char *Bl)
{
    FILE *highest;
    FILE *highLow;
    FILE *lowHigh;
    FILE *lowest;

    //Highest
    if ((highest = fdopen(pipefds[0][1], "w")) == NULL)
    {
        exitError("FILE 'highest' could not be opened for writing.", errno);
    }

    if (fprintf(highest, "%s\n", Ah) < 0)
    {
        exitError("Could not write Ah to readPipe in Parent.", 0);
    }

    if (fprintf(highest, "%s", Bh) < 0)
    {
        exitError("Could not write Bh to readPipe in Parent.", 0);
    }

    if (fclose(highest) == EOF)
    {
        exitError("Highest could not be closed.", errno);
    }

    //Highlow
    if ((highLow = fdopen(pipefds[2][1], "w")) == NULL)
    {
        exitError("FILE 'highLow' could not be opened for writing.", errno);
    }

    if (fprintf(highLow, "%s\n", Ah) < 0)
    {
        exitError("Could not write Ah to readPipe in Parent.", 0);
    }

    if (fprintf(highLow, "%s", Bl) < 0)
    {
        exitError("Could not write Bh to readPipe in Parent.", 0);
    }

    if (fclose(highLow) == EOF)
    {
        exitError("highLow could not be closed.", errno);
    }

    //LowHigh
    if ((lowHigh = fdopen(pipefds[4][1], "w")) == NULL)
    {
        exitError("FILE 'lowHigh' could not be opened for writing.", errno);
    }

    if (fprintf(lowHigh, "%s\n", Al) < 0)
    {
        exitError("Could not write Ah to readPipe in Parent.", 0);
    }

    if (fprintf(lowHigh, "%s", Bh) < 0)
    {
        exitError("Could not write Bh to readPipe in Parent.", 0);
    }

    if (fclose(lowHigh) == EOF)
    {
        exitError("lowHigh could not be closed.", errno);
    }

    //Lowest
    if ((lowest = fdopen(pipefds[6][1], "w")) == NULL)
    {
        exitError("FILE 'lowest' could not be opened for writing.", errno);
    }

    if (fprintf(lowest, "%s\n", Al) < 0)
    {
        exitError("Could not write Ah to readPipe in Parent.", 0);
    }

    if (fprintf(lowest, "%s", Bl) < 0)
    {
        exitError("Could not write Bh to readPipe in Parent.", 0);
    }

    if (fclose(lowest) == EOF)
    {
        exitError("lowest could not be closed.", errno);
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
    }
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
    fprintf(stdout, "%x\n", (int)res);
}

char *buff = NULL;
static void readInput(void)
{
    size_t len = 0;
    ssize_t linesize;
    int arg1filled = 0;
    while ((linesize = getline(&buff, &len, stdin)) != -1)
    {
        if (!arg1filled)
        {
            if (buff[linesize - 1] == '\n')
            {
                buff[linesize - 1] = '\0';
                --linesize;
            }
            if ((argument1 = malloc(linesize + 1)) == NULL)
            {
                exitError("Malloc for argument1 failed.", errno);
            }
            strncpy(argument1, buff, linesize + 1);
            arg1filled++;
        }
        else
        {
            if ((argument2 = malloc(linesize + 1)) == NULL)
            {
                exitError("Malloc for argument2 failed.", errno);
            }
            strncpy(argument2, buff, linesize + 1);
        }
    }

    free(buff);

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
    free(highestVal);
    free(highLowVal);
    free(lowHighVal);
    free(lowestVal);
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