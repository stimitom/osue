#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

char *outfile = NULL;
int writeToFile;
int removeWhiteSpaces;
int caseInsensitive;

void removeSpaces(char *str)
{
    // To keep track of non-space character count
    int count = 0;

    // Traverse the given string. If current character
    // is not space, then place it at index 'count++'
    for (int i = 0; str[i]; i++)
    {
        if (str[i] != ' ')
        {
            str[count++] = str[i];
        }
    }
    str[count] = '\0';
}

void allToLower(char *str)
{
    for (int i = 0; i < strlen(str); i++)
    {
        str[i] = tolower(str[i]);
    }
}

void isPalindrom(char str[])
{
    char initial[strlen(str)];
    strcpy(initial, str);

    if (removeWhiteSpaces)
    {
        removeSpaces(str);
    }

    if (caseInsensitive)
    {
        allToLower(str);
    }

    int l = 0;
    int h = strlen(str) - 1;

    if (writeToFile)
    {
        FILE *output = fopen(outfile, "a");
        if (output == NULL)
        {
            return;
        }
        while (h > l)
        {
            if (str[l++] != str[h--])
            {
                /* Compares characters from beginning and end to the middle. Returns if they are not equal.*/
                strcat(initial, " is not a palindrom.\n");
                fputs(initial, output);
                return;
            }
        }
        strcat(initial, " is a palindrom.\n");
        fputs(initial, output);
    }
    else
    {
        while (h > l)
        {
            if (str[l++] != str[h--])
            {
                printf("%s is not a palindrom.\n", initial);
                return;
            }
        }
        printf("%s is a palindrom.\n", initial);
    }
}

void readFromSource(FILE *source)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t linesize;
    while ((linesize = getline(&line, &len, source)) != -1)
    {
        if (line[linesize - 1] == '\n')
        {
            // removes newline character from input string
            line[linesize - 1] = '\0';
            --linesize;
        }

        isPalindrom(line);
    }

    free(line);
}

int main(int argc, char *argv[])
{
    int opt_num = 0;
    int c;
    while ((c = getopt(argc, argv, "iso:")) != -1)
    {
        switch (c)
        {
        case 'i':
            caseInsensitive = 1;
            opt_num++;
            break;
        case 's':
            removeWhiteSpaces = 1;
            opt_num++;
            break;
        case 'o':
            writeToFile = 1;
            outfile = optarg;
            opt_num += 2;
            break;
        default:
            break;
        }
    }

    if (argc - 1 > opt_num)
    {
        for (size_t i = opt_num + 1; i < argc; i++)
        {
            FILE *input = fopen(argv[i], "r");
            if (input == NULL)
            {
                fprintf(stderr, "fopen failed: %s \n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            readFromSource(input);
        }
    }
    else
    {
        readFromSource(stdin);
    }

    return 0;
}
