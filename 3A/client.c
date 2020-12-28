#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

char *pgmName;
FILE *outfile;
char *url;
long int port = 80;

static void parseArguments(int argc, char *argv[]);
static void usage(void);
static void exitError(char *message, int errnum);

int main(int argc, char *argv[])
{
    pgmName = argv[0];
    parseArguments(argc, argv);
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
            if (o_count || d_count)
            {
                usage();
            }
            d_arg = optarg;
            d_count++;
            numberOfArgs += 2;
            break;
        case 'o':
            if (d_count || o_count)
            {
                usage();
            }
            o_arg = optarg;
            o_count++;
            numberOfArgs += 2;
            break;
        case 'p':
            if (p_count)
            {
                usage();
            }
            p_arg = optarg;
            p_count++;
            numberOfArgs += 2;
            break;
        case '?':
            usage();
            break;
        default:
            break;
        }
    }

    if (numberOfArgs + 2 != argc)
    {
        usage();
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
    char *filePath = strrchr(url, '/');
    if (&filePath[0] == &url[6])
    {
        exitError("The filepath does not specify a resource.", 0);
    }

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
    }else{
        outfile = stdout;
    }

    if (p_count)
    {
        char *endpointer;
        port = strtol(p_arg, &endpointer, 10);
        if (endpointer != NULL)
        {
            if (endpointer == p_arg)
            {
                exitError("The given port number is not valid", 0);
            }
        }
    }

    fprintf(outfile,"URL: %s\n", url);
    fprintf(outfile,"filePath: %s\n", filePath);
    // fprintf(outfile,"outfile: %s\n", localFileName);
    fprintf(outfile,"port: %ld\n", port);
}

static void usage(void)
{
    fprintf(stderr, "ERROR\nUsage: %s [-p PORT] [ -o FILE | -d DIR ] URL\n", pgmName);
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