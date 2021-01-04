#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <sys/stat.h>

char *pgmName;
char *indexFilename;
char *port;
struct addrinfo *ai;
char *docRootPath;
char *requestedFilePath;
FILE *sockfile;
FILE *requestedFile;
volatile sig_atomic_t quit = 0;
volatile sig_atomic_t connOpen = 0;
volatile sig_atomic_t requestedFileOpen = 0;

static void parseArguments(int argc, char *argv[]);
static void readRequest(void);
static int handleRequestHeader(char *buff);
static void sendNegResponseHeader(int status);
static void transmitRequestedFile(void);
static void sendPosResponseHeader(char *completeFilePath);
static void writeRequestedFileToConnection(void);
static void closeConnection(void);
static void handleSignal(int signal);
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

    struct sigaction sa;
    bzero(&sa, sizeof(sa));
    sa.sa_handler = handleSignal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    parseArguments(argc, argv);

    struct addrinfo hints;
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int res = getaddrinfo(NULL, port, &hints, &ai);
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

    int optval = 1;
    if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
    {
        exitError("Sockoption could not be set.", errno);
    }

    if (bind(sockFd, ai->ai_addr, ai->ai_addrlen) == -1)
    {
        exitError("Bind for socket failed.", errno);
    }

    if (listen(sockFd, 1) == -1)
    {
        exitError("Listen for sockfd failed.", errno);
    }

    while (!quit)
    {
        int connFd;
        if ((connFd = accept(sockFd, NULL, NULL)) == -1)
        {
            exitError("Accept for sockfd failed.", errno);
        }

        if ((sockfile = fdopen(connFd, "r+")) == NULL)
        {
            exitError("Sockfile could not be opened.", errno);
        }

        connOpen = 1;

        readRequest();
    }
    closeConnection();

    exit(EXIT_SUCCESS);
}

static void parseArguments(int argc, char *argv[])
{

    int c, p_count = 0, i_count = 0, numberOfArgs = 0;
    char *p_arg = NULL, *i_arg = NULL;
    while ((c = getopt(argc, argv, "p:i:")) != -1)
    {
        switch (c)
        {
        case 'p':
            if (p_count)
            {
                usage("Option 'p' can only be provided once.");
            }
            p_arg = optarg;
            p_count++;
            numberOfArgs += 2;
            break;
        case 'i':
            if (i_count)
            {
                usage("Option 'i' can only be provided once.");
            }
            i_arg = optarg;
            i_count++;
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

    docRootPath = argv[optind];

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
        if ((port = malloc(5)) == NULL)
        {
            exitError("Malloc for port failed.", errno);
        }
        strcpy(port, "8080");
    }

    if (i_count)
    {
        if ((indexFilename = malloc(strlen(i_arg) + 1)) == NULL)
        {
            exitError("Malloc for indexFilename failed.", errno);
        }
        strcpy(indexFilename, i_arg);
    }
    else
    {
        if ((indexFilename = malloc(strlen("index.html") + 1)) == NULL)
        {
            exitError("Malloc for indexFilename failed.", errno);
        }
        strcpy(indexFilename, "index.html");
    }
}

static void readRequest(void)
{
    char *buff = NULL;
    size_t len = 0;
    ssize_t linesize;
    int checkedStatusLine = 0;

    while ((linesize = getline(&buff, &len, sockfile)) != -1)
    {
        if (!checkedStatusLine)
        {
            char helpBuff[linesize + 1];
            strcpy(helpBuff, buff);
            if (!handleRequestHeader(helpBuff))
            {
                break;
            }
            checkedStatusLine++;
        }
        else if (strcmp(buff, "\r\n") == 0)
        {
            transmitRequestedFile();
        }
    }

    free(buff);
}

static int handleRequestHeader(char *buff)
{

    char *token[3];
    token[0] = strtok(buff, " ");
    token[1] = strtok(NULL, " ");
    token[2] = strtok(NULL, "\r");

    if (strtok(NULL, "\n") != NULL)
    {
        sendNegResponseHeader(400);
        return 0;
    }

    if (strcmp(token[0], "GET") != 0)
    {
        sendNegResponseHeader(501);
        return 0;
    }

    if (strcmp(token[2], "HTTP/1.1") != 0)
    {
        sendNegResponseHeader(400);
        return 0;
    }

    if ((requestedFilePath = malloc(strlen(token[1]) + 1)) == NULL)
    {
        exitError("Malloc for requested file path failed.", errno);
    }
    strcpy(requestedFilePath, token[1]);
    return 1;
}

static void sendNegResponseHeader(int status)
{
    char responseHeader[100];
    if (status == 400)
    {
        strcpy(responseHeader, "HTTP/1.1 400 Bad Request\r\n");
    }
    else if (status == 404)
    {
        strcpy(responseHeader, "HTTP/1.1 404 Not Found\r\n");
    }
    else if (status == 501)
    {
        strcpy(responseHeader, "HTTP/1.1 501 Not Implementedr\n");
    }

    strcat(responseHeader, "Connection: close\r\n");

    fprintf(stderr,"%s", responseHeader);

    if (fputs(responseHeader, sockfile) == EOF)
    {
        exitError("Response header could not be written.", 0);
    }

    if (fflush(sockfile) != 0)
    {
        exitError("Sockfile could not be flushed.", errno);
    }

}

static void transmitRequestedFile(void)
{
    char *completeFilePath;
    if ((completeFilePath = malloc(strlen(requestedFilePath) + strlen(docRootPath) + strlen(indexFilename) + 1)) == NULL)
    {
        exitError("Malloc for completeFilePath failed.", errno);
    }
    bzero(completeFilePath, strlen(requestedFilePath) + strlen(docRootPath) + 1);
    strcat(completeFilePath, docRootPath);
    strcat(completeFilePath, requestedFilePath);
    if (requestedFilePath[strlen(requestedFilePath) - 1] == '/')
    {
        strcat(completeFilePath, indexFilename);
    }

    if ((requestedFile = fopen(completeFilePath, "r")) == NULL)
    {
        sendNegResponseHeader(404);
    }
    else
    {   
        requestedFileOpen = 1;
        sendPosResponseHeader(completeFilePath);
        writeRequestedFileToConnection();
    }

    free(completeFilePath);
}

static void sendPosResponseHeader(char *completeFilePath)
{
    char date[50];
    time_t t;
    struct tm *tmp;

    t = time(NULL);
    tmp = localtime(&t);
    if (tmp == NULL)
    {
        exitError("Localtime failed.", 0);
    }

    if (strftime(date, sizeof(date), "%a, %d %b %y %T %z", tmp) == 0)
    {
        exitError("strftime failed.", 0);
    }

    struct stat st;
    stat(completeFilePath, &st);
    off_t fSize = st.st_size;
    char *fileSize;
    if ((fileSize = malloc(100)) == NULL)
    {
        exitError("Malloc for fileSize failed.", errno);
    }
    bzero(fileSize, 100);
    sprintf(fileSize, "%ld", fSize);

    char *responseHeader;
    if ((responseHeader = malloc(strlen(date) + strlen(fileSize) + 70)) == NULL)
    {
        exitError("Malloc for request failed.", 0);
    }

    bzero(responseHeader, strlen(date) + strlen(fileSize) + 70);
    sprintf(responseHeader, "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Length: %s\r\nConnection: close\r\n\r\n", date, fileSize);
    free(fileSize);

    fprintf(stderr, "%s", responseHeader);

    if (fputs(responseHeader, sockfile) == EOF)
    {
        exitError("Response Header could not be written.", 0);
    }

    if (fflush(sockfile) != 0)
    {
        exitError("Sockfile could not be flushed.", errno);
    }

    free(responseHeader);
}

static void writeRequestedFileToConnection(void)
{
    char *buff = NULL;
    size_t len = 0;
    ssize_t linesize;
    while ((linesize = getline(&buff, &len, requestedFile)) != -1)
    {
        if (fputs(buff, sockfile) == EOF)
        {
            exitError("Requested File could not be written.", 0);
        }
    }

    if (fflush(sockfile) != 0)
    {
        exitError("Sockfile could not be flushed.", errno);
    }

    free(buff);
}

// Utility functions

static void closeConnection(void)
{
    if (fclose(sockfile) == EOF)
    {
        exitError("Sockfile could not be closed.", errno);
    }
    connOpen = 0;

    if (requestedFileOpen)
    {
        if (fclose(requestedFile) == EOF)
        {
            exitError("Requested file could not be closed.", errno);
        }
    }
}

static void usage(char *message)
{
    fprintf(stderr, "[%s]: %s\n", pgmName, message);
    fprintf(stderr, "Usage: %s [-p PORT] [-i INDEX] DOC_ROOT\n", pgmName);
    exit(EXIT_FAILURE);
}

static void handleSignal(int signal)
{
    if (!connOpen)
    {
        _exit(0);
    }
    quit = 1;
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
    free(requestedFilePath);
    free(indexFilename);
    freeaddrinfo(ai);
    free(port);
}