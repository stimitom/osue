/**
 * @file client.c
 * @author Thomas Stimakovits <0155190@student.tuwien.ac.at>
 * @date 30.12.2020
 *
 * @brief A partial implementation of an http client.
 * 
 * @details This program can perform GET requests to an Http Server.
 **/

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

char *pgmName;
FILE *outfile;
char *url;
char *hostName;
char *filePath;
char *port;

static void parseArguments(int argc, char *argv[]);
static void parseResponseHeader(char *responseHeader);
static void protocolError(void);
static void usage(char *message);
static void exitError(char *message, int errnum);
static void cleanUp(void);

/**
 * Main-function. Program entry point.
 * @brief The socket for sending http requests is created. A request is sent and the response is read.
 * @details 
 * The exit function is set.
 * The argument parsing is delegated to parseArguments(). 
 * The socket is setup with the given port. 
 * After the connection is established the sockfile is opened. 
 * A GET-request is created and written to the connection. 
 * After sending the request the response is read and written to the indicated outfile (or stdout).
 * Open buffers are freed and the connection is closed.
 * @return EXIT_SUCCESS OR EXIT_FAILURE
 */
int main(int argc, char *argv[])
{
    pgmName = argv[0];
    if (atexit(cleanUp) != 0)
    {
        exitError("atexit failed.", 0);
    }
    parseArguments(argc, argv);

    struct addrinfo hints, *ai;
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int res = getaddrinfo(hostName, port, &hints, &ai);
    if (res != 0)
    {
        fprintf(stderr, "[%s]: %s\n", pgmName, gai_strerror(res));
        exit(EXIT_FAILURE);
    }

    int sockFd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sockFd == -1)
    {
        exitError("Socket could not be created.", errno);
    }

    if (connect(sockFd, ai->ai_addr, ai->ai_addrlen) == -1)
    {
        exitError("Connect failed.", errno);
    }

    FILE *sockfile;
    if ((sockfile = fdopen(sockFd, "r+")) == NULL)
    {
        exitError("Sockfile could not be opened.", errno);
    }

    char *request;
    if ((request = malloc(strlen(filePath) + strlen(hostName) + 100)) == NULL)
    {
        exitError("Malloc for request failed.", 0);
    }

    bzero(request, strlen(filePath) + strlen(hostName) + 100);
    sprintf(request, "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", filePath, hostName);
    if (fputs(request, sockfile) == EOF)
    {
        exitError("Request could not be written.", 0);
    }
    if (fflush(sockfile) != 0)
    {
        exitError("Sockfile could not be flushed.", errno);
    }

    char *buff = NULL;
    size_t len = 0;
    ssize_t linesize;

    int checkedStatusLine = 0;
    int pastHeaders = 0;

    while ((linesize = getline(&buff, &len, sockfile)) != -1)
    {
        if (!checkedStatusLine)
        {
            char *helpBuff;
            if ((helpBuff = malloc(linesize + 1)) == NULL)
            {
                exitError("Malloc for helpBuff failed.", errno);
            }
            strcpy(helpBuff, buff);
            parseResponseHeader(helpBuff);
            free(helpBuff);
            checkedStatusLine++;
        }
        if (pastHeaders)
        {
            fputs(buff, outfile);
        }
        if (strcmp(buff, "\r\n") == 0)
        {
            pastHeaders++;
        }
    }
    free(buff);
    freeaddrinfo(ai);
    free(request);

    if (fclose(sockfile) != 0)
    {
        exitError("Sockfile could not be closed.", errno);
    }
}
/**
 * parseArguments
 * @brief The input arguments are handled.
 * @param int argc 
 * @param char *argv[]
 * @details It is checked if the possible options are used correctly and if the compulsory argument URL is provided.
 * If option d is given, the outfile is set to the given directory using the specified indexFilename.
 * If option o is given, the name of outfile is set to the provided string else the name is set to "index.html".
 * If neither d or o are provided the stdout is used as the outfile.
 * If option p is given, the port number is checked and set to the provided number else the port number is set to 80.
 * The URL is checked if it starts with "http://".
 * @return void
 */
static void parseArguments(int argc, char *argv[])
{
    char *d_arg = NULL, *o_arg = NULL, *p_arg = NULL;
    int d_count = 0, o_count = 0, p_count = 0, numberOfArgs = 0, c;
    while ((c = getopt(argc, argv, "d:o:p:")) != -1)
    {
        switch (c)
        {
        case 'd':
            if (o_count)
            {
                usage("Option d and o cannot be used both.");
            }
            if (d_count)
            {
                usage("Option d provided more than once.");
            }
            d_arg = optarg;
            d_count++;
            numberOfArgs += 2;
            break;
        case 'o':
            if (d_count)
            {
                usage("Option d and o cannot be used both.");
            }
            if (o_count)
            {
                usage("Option o provided more than once.");
            }
            o_arg = optarg;
            o_count++;
            numberOfArgs += 2;
            break;
        case 'p':
            if (p_count)
            {
                usage("Option p provided more than once.");
            }
            p_arg = optarg;
            p_count++;
            numberOfArgs += 2;
            break;
        case '?':
            usage("Wrong option provided.");
            break;
        default:
            break;
        }
    }

    if (numberOfArgs + 2 != argc)
    {
        usage("Wrong amount of arguments provided.");
    }
    url = argv[optind];
    if (url[0] != 'h' ||
        url[1] != 't' ||
        url[2] != 't' ||
        url[3] != 'p' ||
        url[4] != ':' ||
        url[5] != '/' ||
        url[6] != '/')
    {
        exitError("The specified URL does start with the required protocol 'http://' .", 0);
    }

    if ((filePath = malloc(strlen(url) - 6)) == NULL)
    {
        exitError("Malloc for filePath failed.", errno);
    }
    strcpy(filePath, &url[7]);

    char *helpHostName;
    if ((helpHostName = malloc(strlen(url) - 6)) == NULL)
    {
        exitError("Malloc for helpHostName2 failed.", errno);
    }
    strcpy(helpHostName, filePath);
    hostName = filePath;
    strsep(&filePath, ";/?:@=&");
    int indexOfSep = (&filePath[0] - &hostName[0]) - 1;

    if (helpHostName[indexOfSep] != '/')
    {
        exitError("The given URL is not correct.", 0);
    }

    free(helpHostName);

    char *localFileName;
    if (d_count)
    {
        if (url[strlen(url) - 1] == '/')
        {
            if ((localFileName = malloc(strlen(d_arg) + strlen("/index.html") + 1)) == NULL)
            {
                exitError("Malloc for localfilename failed.", errno);
            }
            bzero(localFileName, sizeof(localFileName));
            strcat(localFileName, d_arg);
            strcat(localFileName, "/index.html");
        }
        else
        {
            char *fName = strrchr(filePath, '/');
            if ((localFileName = malloc(strlen(d_arg) + strlen(fName) + 1)) == NULL)
            {
                exitError("Malloc for localfilename failed.", errno);
            }
            bzero(localFileName, sizeof(localFileName));
            strcat(localFileName, d_arg);
            strcat(localFileName, fName);
        }
    }
    else if (o_count)
    {
        if ((localFileName = malloc(strlen(o_arg) + 1)) == NULL)
        {
            exitError("Malloc for localfilename failed.", errno);
        }
        strcpy(localFileName, o_arg);
    }
    if (o_count || d_count)
    {
        if ((outfile = fopen(localFileName, "a")) == NULL)
        {
            exitError("The output file could not be opened.", errno);
        }
        free(localFileName);
    }
    else
    {
        outfile = stdout;
    }

    if (p_count)
    {
        for (int i = 0; i < strlen(p_arg); i++)
        {
            if (p_arg[i] < '0' || p_arg[i] > '9')
            {
                usage("Port number not valid.");
            }
        }
        if ((port = malloc(strlen(p_arg) + 1)) == NULL)
        {
            exitError("Malloc for port failed.", errno);
        }
        strcpy(port, p_arg);
    }
    else
    {
        if ((port = malloc(3)) == NULL)
        {
            exitError("Malloc for port failed.", errno);
        }
        strcpy(port, "80");
    }
}

/**
 * parseResponseHeader
 * @brief A given responseHeader is analysed.
 * @param char *responseHeader - a copy of the response Header (not the reference to the original response Header!)
 * @details The response Header is split into two parts using strtok
 *  and references to each part are stored in the char-pointer array "token[2]".
 *  It is checked if the response header 
 *      has "HTTP/1.1" as its first part. Else a protocol error is written to stderr. 
 *      has a readable response status as its second part. Else a protocol error is written to stderr.
 *      has 200 as response status. Else The negative response status is written to stderr and the programm exits with error code 3. 
 * @return void
 */
static void parseResponseHeader(char *responseHeader)
{
    char *token[2];
    token[0] = strtok(responseHeader, " ");
    token[1] = strtok(NULL, " ");

    if (strcmp(token[0], "HTTP/1.1") != 0)
    {
        protocolError();
    }

    long int responseStatus;
    char *endpointer;
    responseStatus = strtol(token[1], &endpointer, 10);
    if (endpointer != NULL)
    {
        if (*endpointer != '\0')
        {
            protocolError();
        }
    }

    if (responseStatus != 200)
    {
        char *ptr;
        char *statusMessage;
        if ((statusMessage = malloc(100)) == NULL)
        {
            exitError("Malloc for status message failed.", errno);
        }
        int size = 0;
        while ((ptr = strtok(NULL, " ")) != NULL)
        {
            if (size + strlen(ptr) < 99)
            {
                strcat(statusMessage, ptr);
                size += strlen(ptr);
            }
        }
        fprintf(stderr, "[%s]: Status %ld: %s \n", pgmName, responseStatus, statusMessage);
        free(statusMessage);
        exit(3);
    }
}

/**
 * protocolError
 * @brief Prints a formatted protocol error message to stderr and exits with error code 2.
 * @return void
 */
static void protocolError(void)
{
    fprintf(stderr, "[%s]: Protocol error!\n", pgmName);
    exit(2);
}

// Utility functions

/**
 * usage
 * @brief Prints the given error message and the usage message formatted to stderr. Exits with EXIT FAILURE.
 * @param char *message - the error message
 * @return void
 */
static void usage(char *message)
{
    fprintf(stderr, "[%s]: %s\n", pgmName, message);
    fprintf(stderr, "Usage: %s [-p PORT] [ -o FILE | -d DIR ] URL\n", pgmName);
    exit(EXIT_FAILURE);
}

/**
 * exitError
 * @brief Custom Exit Function to print formatted error message and exit.
 * @param char *message - the error message
 * @param int errnumm - for using the errno in the error message output. 0 if errno description is not needed.
 * @return void
 */
static void exitError(char *message, int errnum)
{
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

/**
 * cleanUp
 * @brief frees allocated memory areas amd closes the outfile.
 * @return void
 */
static void cleanUp(void)
{
    filePath -= (strlen(hostName) + 1);
    free(filePath);
    free(port);
    if (outfile != stdout)
    {
        fclose(outfile);
    }
}