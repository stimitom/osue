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
char *filePath;
char *hostName;
char *helpHostName;
char *port;

static void parseArguments(int argc, char *argv[]);
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

    // struct addrinfo hints, *ai;
    // bzero(&hints, sizeof(hints));
    // hints.ai_family = AF_UNSPEC;
    // hints.ai_socktype = SOCK_STREAM;

    // int res = getaddrinfo(hostName, port, &hints, &ai);
    // if (res != 0)
    // {
    //     exitError(gai_strerror(res), 0);
    // }

    // int sockFd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    // if (sockFd == -1)
    // {
    //     exitError("Socket could not be created.", errno);
    // }

    // if (connect(sockFd, ai->ai_addr, ai->ai_addrlen) == -1)
    // {
    //     exitError("Connect failed.", errno);
    // }

    // FILE *sockfile;
    // if ((sockfile = fdopen(sockFd, "r+")) == NULL)
    // {
    //     exitError("Sockfile could not be opened.", errno);
    // }
    // printf("Pathfile: %s\n", filePath);

    // fputs("GET ",sockfile);
    // fputs("GET / HTTP/1.1\r\nHost: ", sockfile);
    // fputs(hostName, sockfile);
    // fputs("\r\nConnection: close",sockfile);
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
            if(d_count){
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
            if(o_count){
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

    filePath = strrchr(url, '/');
    if (&filePath[0] == &url[6])
    {
        exitError("The filepath does not specify a resource.", 0);
    }

    if ((helpHostName = malloc(strlen(url) + 1)) == NULL)
    {
        exitError("Malloc for helpHostName failed.", errno);
    }
    strcpy(helpHostName, url);
    helpHostName += 7;
    hostName = helpHostName;
    strsep(&helpHostName, ";/?:@=&");
    helpHostName -= 7;
    helpHostName -= (strlen(hostName) + 1);

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
        if((port = malloc(strlen(p_arg)+1)) == NULL){
            exitError("Malloc for port failed.", errno);
        }
        strcpy(port,p_arg);  
    }else{
        if((port = malloc(3)) == NULL){
            exitError("Malloc for port failed.", errno);
        }
        strcpy(port,"80");  
    }
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
    free(helpHostName);
    free(port);
}