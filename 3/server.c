/**
 * @file server.c
 * @author Thomas Stimakovits <0155190@student.tuwien.ac.at>
 * @date 04.01.2021
 *
 * @brief A partial implementation of an http-server.
 * 
 * @details This program answers to GET requests transmitted using the http-protocol.
 * 
 **/

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
FILE *sockfile;
char negativeResponseHeader[100];
volatile sig_atomic_t quit = 0;
volatile sig_atomic_t connOpen = 0;

static void parseArguments(int argc, char *argv[]);
static void handleRequest(void);
static int handleRequestHeader(char *requestHeader, char **requestedFilePathAdr);
static void createNegResponseHeader(int status);
static void transmitRequestedFile( char *requestedFilePath);
static void sendPosResponseHeader(char *completeFilePath);
static void writeRequestedFileToConnection(FILE *requestedFile);
static void sendNegResponseHeader(void);
static void closeConnection(void);
static void handleSignal(int signal);
static void usage(char *message);
static void exitError(char *message, int errnum);
static void cleanUp(void);

/**
 * Main-function. Program entry point.
 * @brief The socket for receiving http requests is created. The socket listens for incoming requests until a signal interrupts it.
 * @details 
 * The signal handling and exit functions are set.
 * The argument parsing is delegated to parseArguments(). 
 * The socket is setup and starts listening on the specified port. 
 * As long as no signal (SIGINT, SIGTERM) interrupts, one incoming connection is accepted. 
 * The reading of an request is delegated to handleRequest(). 
 * After handling the request the connection is closed and a new connection can be accepted.
 * @return EXIT_SUCCESS OR EXIT_FAILURE
 */
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

        handleRequest();
        closeConnection();
    }

    exit(EXIT_SUCCESS);
}

/**
 * parseArguments
 * @brief The input arguments are handled.
 * @param int argc 
 * @param char *argv[]
 * @details It is checked if the possible options are used correctly and if the compulsory argument DOC_ROOT is provided.
 * If option p is given, the port number is checked and set to the provided number else the port number is set to 8080.
 * If option i is given, the name of index file is set to the provided string else the name is set to "index.html".
 * @return void
 */
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

/**
 * handleRequest - core function
 * @brief The incoming request-header and -body are read and an according response is sent.
 * @details 
 * It is checked wheter the incoming request header is properly formed in handleRequestHeader and the answer is stored in requestHeaderOk.
 * After reading through the whole request (this happens when the buffer is equal to "\r\n") an 
 * answer is sent. 
 * If the requestHeader was properly formed transmitRequestedFile() is called to send the requested File.
 * Else a negative resonse is sent in sendNegResponseHeader().
 * @return void
 */
static void handleRequest(void)
{
    char *buff = NULL;
    size_t len = 0;
    ssize_t linesize;
    int checkedStatusLine = 0;
    int requestHeaderOk = 0;
    char *requestedFilePath = NULL;
    while ((linesize = getline(&buff, &len, sockfile)) != -1)
    {
        if (!checkedStatusLine)
        {
            char helpBuff[linesize + 1];
            strcpy(helpBuff, buff);
            requestHeaderOk = handleRequestHeader(helpBuff, &requestedFilePath);
            checkedStatusLine++;
        }
        else if (strcmp(buff, "\r\n") == 0)
        {
            if (requestHeaderOk)
            {
                transmitRequestedFile(requestedFilePath);
            }
            else
            {
                sendNegResponseHeader();
            }
            break;
        }
    }
    free(requestedFilePath);
    free(buff);
}

/**
 * handleRequestHeader
 * @brief A given requestHeader is analysed.
 * @param char *requestHeader - a copy of the request Header (not the reference to the original request Header!)
 * @param char **requestedFilePathAddr - address of requestedFilePath
 * @details The request Header is split into three parts using strtok
 *  and references to each part are stored in the char -pointer array "token[3]".
 *  It is checked if the request header 
 *      contains just the required parts. Else a negative response header with status 400 is created.
 *      has "GET" as its first part. Else a negative response header with status 501 is created. 
 *      has "HTTP/1.1" as its second part. Else a negative response header with status 400 is created.
 * The filePath is taken as is and stored in the dereferenced requestedFilePathAdr (= requestedFilePath).
 * @return 1 if request Header Ok . 0 if erroneous.
 */
static int handleRequestHeader(char *requestHeader, char **requestedFilePathAdr)
{
    char *token[3];
    token[0] = strtok(requestHeader, " ");
    token[1] = strtok(NULL, " ");
    token[2] = strtok(NULL, "\r");

    if (strtok(NULL, "\n") != NULL)
    {
        createNegResponseHeader(400);
        return 0;
    }

    if (strcmp(token[0], "GET") != 0)
    {
        createNegResponseHeader(501);
        return 0;
    }

    if (strcmp(token[2], "HTTP/1.1") != 0)
    {
        createNegResponseHeader(400);
        return 0;
    }

    if ((*requestedFilePathAdr = malloc(strlen(token[1]) + 1)) == NULL)
    {
        exitError("Malloc for requested file path failed.", errno);
    }
    strcpy(*requestedFilePathAdr, token[1]);
    return 1;
}

/**
 * createNegResponseHeader
 * @brief A negative Response Header is created and stored in a global variable according to a given status.
 * @param int *status - the error status
 * @details A negative Response Header is created and stored in a global variable according to a given status. 
 * The Connection field is added with the value 'close' to the negative reponse.
 * @return void
 */
static void createNegResponseHeader(int status)
{
    if (status == 400)
    {
        strcpy(negativeResponseHeader, "HTTP/1.1 400 Bad Request\r\n");
    }
    else if (status == 404)
    {
        strcpy(negativeResponseHeader, "HTTP/1.1 404 Not Found\r\n");
    }
    else if (status == 501)
    {
        strcpy(negativeResponseHeader, "HTTP/1.1 501 Not Implementedr\n");
    }

    strcat(negativeResponseHeader, "Connection: close\r\n");
}

/**
 * transmitRequestedFile
 * @brief The requested File is opened and if available written to the socket. Else a negative response is sent.
 * @param char *requestedFilePath
 * @details The complete file path is created from the given requesteFile and the document root path. 
 * If the requested file path is a directory the index file name is appended to the complete file path.
 * The requested file is opened: 
 *      if it fails (this means the file is not available or existing) a negative response with status 404 is sent.
 *      if it succeeds a positive response header is sent. After that the requested file's content is sent 
 *          in writeRequestedFileToConnection() , then the transmitted file is closed again.
 * @return void
 */
static void transmitRequestedFile(char *requestedFilePath)
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

    FILE *requestedFile;
    if ((requestedFile = fopen(completeFilePath, "r")) == NULL)
    {
        createNegResponseHeader(404);
        sendNegResponseHeader();
    }
    else
    {
        sendPosResponseHeader(completeFilePath);
        writeRequestedFileToConnection(requestedFile);
        if (fclose(requestedFile) == EOF)
        {
            exitError("Requested file could not be closed.", errno);
        }
    }

    free(completeFilePath);
}

/**
 * sendNegativeResponseHeader
 * @brief The globally stored negative reponse is sent.
 * @details The negative response is written and flushed.
 * @return void
 */
static void sendNegResponseHeader(void)
{
    if (fputs(negativeResponseHeader, sockfile) == EOF)
    {
        exitError("Response header could not be written.", 0);
    }

    fprintf(stderr, "%s\n", negativeResponseHeader);

    if (fflush(sockfile) != 0)
    {
        exitError("Sockfile could not be flushed.", errno);
    }
}

/**
 * sendPosResponseHeader
 * @brief A positive response Header is created and sent.
 * @param char *completeFilePath - path to the file that is to be transmitted.
 * @details Using strftime a formatted date-time as secified in RFC 822 is created.
 * Using stat the filesize of the file to be transmitted is found.
 * A positive Response header is created written to the connection and flushed.
 * @return void
 */
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

/**
 * writeRequestedFileToConnection
 * @brief A given File is read and written to the connection line by line.
 * @param FILE *requestedFile
 * @details The file is written to the connection and flushed.
 * @return void
 */
static void writeRequestedFileToConnection(FILE *requestedFile)
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

/**
 * closeConnection
 * @brief Closes the connection and sets the volatile connOpen to 0.
 * @return void
 */
static void closeConnection(void)
{
    if (fclose(sockfile) == EOF)
    {
        exitError("Sockfile could not be closed.", errno);
    }
    connOpen = 0;
}

/**
 * usage
 * @brief Prints the given error message and the usage message formatted to stderr. Exits with EXIT FAILURE.
 * @param char *message - the error message
 * @return void
 */
static void usage(char *message)
{
    fprintf(stderr, "[%s]: %s\n", pgmName, message);
    fprintf(stderr, "Usage: %s [-p PORT] [-i INDEX] DOC_ROOT\n", pgmName);
    exit(EXIT_FAILURE);
}

/**
 * handleSignal
 * @brief The signal handler.
 * @param int signal
 * @details If a connection is open the quit flag is set (the request will still be orderly answered), else the programm exits immediately.
 * @return void
 */
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
 * @brief frees allocated memory areas.
 * @return void
 */
static void cleanUp(void)
{
    free(indexFilename);
    freeaddrinfo(ai);
    free(port);
}