#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

int main(int argc, char *argv[])
{
    char *input = NULL;
    size_t len = 0;
    while (!feof(stdin))
    {
        ssize_t linesize = getline(&input, &len, stdin);
        if (input[linesize - 1] == '\n')
        {
            // removes newline character from input string
            input[linesize - 1] = '\0';
            --linesize;
        }

        isPalindrom(input);
    }

    free(input);

    return 0;
}
