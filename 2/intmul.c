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
static void executeChildProcess(void);
static void readFromChildren(void);
static void appendZeroes(char *initial, int numberOfDigits);
static void addPartSolutions(char *highest, char *highlow, char *lowhigh, char *lowest);
static int hexCharToInt(char character);
static void fromIntToHexChar(int i, char *c);
static char *reverseString(char *str);
static void addHexStrings(char *a, char *b, char *endResult);

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
        //Parent
        closeUnneccasaryPipeEndsParent();
        writeToPipes();
        waitForChildren();
        readFromChildren();
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

static void addHexStrings(char *a, char *b, char *endResult)
{
    char *first;
    char *second;
    // Length of second needs to be > length of first
    if (strlen(a) > strlen(b))
    {
        second = malloc(strlen(a));
        strncpy(second, a, strlen(a));
        first = malloc(strlen(b));
        strncpy(first, b, strlen(b));
    }
    else
    {
        second = malloc(strlen(b));
        strncpy(second, b, strlen(b));
        first = malloc(strlen(a));
        strncpy(first, a, strlen(a));
    }

    int n1 = strlen(first), n2 = strlen(second);
    int diff = n2 - n1;
    int resultLen = 0;

    // Take an empty string for storing result
    char result[(n1 + n2)];
    memset(result, 0, (n1 + n2));

    // Initially take carry zero
    char *carry = malloc(1);
    carry[0] = '0';

    char *temp;

    temp = malloc(1);
    // ERror handling

    // Traverse from end of both strings
    for (int i = n1 - 1; i >= 0; i--)
    {
        int sum = hexCharToInt(first[i]) + hexCharToInt(second[i + diff]) + hexCharToInt(carry[0]);
        fromIntToHexChar((sum % 16), temp);
        strncat(result, temp, 1);
        fromIntToHexChar((sum / 16), carry);
        resultLen++;
    }

    // Add remaining digits of str2[]
    char *temp2;
    temp2 = malloc(1);
    //Error handling

    for (int i = n2 - n1 - 1; i >= 0; i--)
    {
        int sum = hexCharToInt(second[i]) + hexCharToInt(carry[0]);

        fromIntToHexChar((sum % 16), temp2);

        strncat(result, temp2, 1);
        fromIntToHexChar((sum / 16), carry);
        resultLen++;
    }

    // Add remaining carry
    if (carry[0])
    {
        strcat(result, carry);
        resultLen++;
    }

    if (realloc(endResult, resultLen) == NULL)
    {
        exitError("Realloc for endresult failed.", errno);
    }
    strncpy(endResult, reverseString(result), resultLen);
}

static void addPartSolutions(char *highest, char *highlow, char *lowhigh, char *lowest)
{
    char *total = malloc(1);
    addHexStrings(highest, highlow, total);
    char *total2 = malloc(1);
    addHexStrings(total, lowhigh, total2);
    char *total3 = malloc(1);
    addHexStrings(total2, lowest, total3);

    fprintf(stdout,"%s\n",total3);
    exit(EXIT_SUCCESS);
}

static void appendZeroes(char *initial, int numberOfDigits)
{
    int oldlength = strlen(initial);
    if (realloc(initial, (oldlength + numberOfDigits) * sizeof(char)) == NULL)
    {
        exitError("Realloc for char 'initial' failed.", errno);
    }

    for (int i = 0; i < numberOfDigits; i++)
    {
        initial[i + oldlength] = '0';
    }
}

static void readFromChildren(void)
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

    char *highestVal = NULL;
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

    char *highLowVal = NULL;
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

    char *lowHighVal = NULL;
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

    char *lowestVal = NULL;
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

    appendZeroes(highestVal, strlen(argument1));
    appendZeroes(highLowVal, strlen(argument1) / 2);
    appendZeroes(lowHighVal, strlen(argument1) / 2);
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
    FILE *highest;
    FILE *highLow;
    FILE *lowHigh;
    FILE *lowest;

    //Highest
    if ((highest = fdopen(pipefds[0][1], "w")) == NULL)
    {
        exitError("FILE 'highest' could not be opened for writing.", errno);
    }

    if(fprintf(highest,"%s\n",Ah) < 0){
        exitError("Could not write Ah to readPipe in Parent.", 0);
    }

     if(fprintf(highest,"%s",Bh) < 0){
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

    if(fprintf(highLow,"%s\n",Ah) < 0){
        exitError("Could not write Ah to readPipe in Parent.", 0);
    }

     if(fprintf(highLow,"%s",Bl) < 0){
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

    if(fprintf(lowHigh,"%s\n",Al) < 0){
        exitError("Could not write Ah to readPipe in Parent.", 0);
    }

     if(fprintf(lowHigh,"%s",Bh) < 0){
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

    if(fprintf(lowest,"%s\n",Al) < 0){
        exitError("Could not write Ah to readPipe in Parent.", 0);
    }

     if(fprintf(lowest,"%s",Bl) < 0){
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