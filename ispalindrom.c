#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

void isPalindrom(char str[])
{
    int l = 0;
    int h = strlen(str) - 1;

    while (h > l)
    {
        if (str[l++] != str[h--])
        {
            /* Compares characters from beginning and end to the middle. Returns if they are not equal.*/
            printf("%s is not a palindrom.\n", str);
            return;
        }
    }
    printf("%s is a palindrom.\n", str);
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
    if (argc > 1)
    {
        for (size_t i = 1; i < argc; i++)
        {
            FILE *input = fopen(argv[i], "r");
            if (input == NULL)
            {
                fprintf(stderr, "fopen failed: %s \n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            readFromSource(input);
            /* code */
        }
    }
    else
    {
        readFromSource(stdin);
    }

    return 0;
}
