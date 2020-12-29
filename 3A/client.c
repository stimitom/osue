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
static void parseResponseHeader(char *buff);
static void protocolError(void);
static void usage(char *message);
static void exitError(char *message, int errnum);
static void cleanUp(void);

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
        if(strcmp(buff,"\r\n") == 0){
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
                usage("Option o provided more than once.");
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
        usage("Too many arguments provided.");
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
            strcat(localFileName, d_arg);
            strcat(localFileName, "/index.html");
        }
        else
        {
            if ((localFileName = malloc(strlen(d_arg) + strlen(filePath) + 1)) == NULL)
            {
                exitError("Malloc for localfilename failed.", errno);
            }
            strcat(localFileName, d_arg);
            strcat(localFileName, filePath);
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

static void parseResponseHeader(char *buff)
{
    char *token[3];
    token[0] = strtok(buff, " ");
    token[1] = strtok(NULL, " ");
    token[2] = strtok(NULL, " ");

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
        fprintf(stderr, "[%s]: Status %ld: %s \n", pgmName, responseStatus, token[2]);
        exit(3);
    }
}

static void protocolError(void)
{
    fprintf(stderr, "[%s]: Protocol error!\n", pgmName);
    exit(2);
}

static void usage(char *message)
{
    fprintf(stderr, "[%s]: %s \n", pgmName, message);
    fprintf(stderr, "Usage: %s [-p PORT] [ -o FILE | -d DIR ] URL\n", pgmName);
    exit(EXIT_FAILURE);
}

/**
 * exitError
 * @brief Custom exit Function.
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