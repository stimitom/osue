#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

char *pgmName;
FILE *outfile;

static void usage(void);
static void exitError(char *message, int errnum);

int main(int argc, char *argv[])
{
    pgmName = argv[0];
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
            break;
        case 'o':
            if (d_count || o_count)
            {
                usage();
            }
            o_arg = optarg;
            o_count++;
            break;
        case 'p':
            if (p_count)
            {
                usage();
            }
            p_arg = optarg;
            p_count++;
            
            break;
        case '?':
            usage();
            break;
        default:
            break;
        }
    }

   printf("%d\n", argc);
   printf("%s\n", argv[optind]);

    if (d_count)
    {
        if (d_arg[strlen(d_arg) - 1] == '/')
        {
            if ((outfile = fopen("index.html", "a")) == NULL)
            {
                exitError("The output file could not be opened.", errno);
            }
        }
    }
    else if (o_count)
    {
        if ((outfile = fopen(o_arg, "a")) == NULL)
        {
            exitError("The output file could not be opened.", errno);
        }
    }

    if (p_count)
    {
    }
}

static void usage(void)
{
    fprintf(stderr, "Usage: %s [-p PORT] [ -o FILE | -d DIR ] URL", pgmName);
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