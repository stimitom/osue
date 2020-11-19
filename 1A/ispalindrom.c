/**
 * @file ispalindrom.c
 * @author Thomas Stimakovits <0155190@student.tuwien.ac.at>
 * @date 18.11.2020
 *
 * @brief Main program module for ispalindrom.
 * 
 * @details This program takes an input from either stdin or a file, checks if it is a palindrom 
 * and prints the answer to a file or stdout.
 * 
 **/

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


/**
 *  removeSpaces function. Help function.
 * @brief This function takes a string as input and returns the same string without any spaces in between. 
 * @details Uses a count to keep track of non-space characters. It then goes through the given string and 
 *  stops if it reaches the null character.
 *  If the current character is not space, it places the charcater at the strings index 'count' 
 *  and then increases the count. That way the spaces get overjumped.
 *  The last character is set to the null-character at the end.
 * @param str - a string (character pointer) to a string to remove spaces from
 * @return void
 */
static void removeSpaces(char *str)
{
    int count = 0;

    for (int i = 0; str[i]; i++)
    {
        if (str[i] != ' ')
        {
            str[count++] = str[i];
        }
    }
    str[count] = '\0';
}

/**
 *  allToLower function. Help function.
 * @brief This function takes a string as input and returns the same string with only lowercase letters.
 * @details uses the tolower-method on each character and returns the whole string.
 * @param str - a string (character pointer) to a string of which all character should be set to lowercase letters.
 * @return void
 */
static void allToLower(char *str)
{
    for (int i = 0; i < strlen(str); i++)
    {
        str[i] = tolower(str[i]);
    }
}

/**
 * isPalindrom function. This programms core function.
 * @brief This function takes an input and returns wheter it is a pa palindrom.
 * @details The input string is stored as it is to a separate space. 
 *  After that the flags are checked (removeWhiteSpaces, caseInsensitive), which change the input string 
 *  according to their function names.
 *  If the writeToFile flag is set the output will not be printed to stdout but written to a given file.
 *  The algorithm for checking if something is apalindrom works by comparing the input's characters from beginning 
 *  and end to the middle. It returns the negative message if one pair is not equal and the positive message otherwise.
 * @param str - a string (character pointer) to be checked for if it is a palindrom
 * @return void
 */
static void isPalindrom(char str[])
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
            fprintf(stderr, "ispalindrom: fopen failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        while (h > l)
        {
            if (str[l++] != str[h--])
            {
                strcat(initial, " is not a palindrom\n");
                if (fputs(initial, output) == EOF)
                {
                    fprintf(stderr, "ispalindrom: fputs failed: %s \n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                return;
            }
        }
        strcat(initial, " is a palindrom\n");
        if (fputs(initial, output) == EOF)
        {
            fprintf(stderr, "ispalindrom: fputs failed: %s \n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        while (h > l)
        {
            if (str[l++] != str[h--])
            {
                printf("%s is not a palindrom\n", initial);
                return;
            }
        }
        printf("%s is a palindrom\n", initial);
    }
}

/**
 * readFromSource function.
 * @brief This function reads line by line from an input source and calls isPalindrom with the line.
 * @details Getline manages the Buffering for the read line. While there are still lines left in the
 * input isPalindrom() is called with the line. After the usage the allocated memory space is freed. 
 * @param source - an input source (File or StdIn) to be read.
 * @return void
 */
static void readFromSource(FILE *source)
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

/**
 * Program entry point.
 * @brief Parses the options and performs and calls helper-functions accordingly. 
 * @details If there are options given to the call, getopt is used to check which option (could be more than one)
 * was provided. 
 * If a file that should be read was provided the file is openend and handed as a parameter to readFromSource().
 * Otherwise StdIn is handed as a paramter to readFromSource().
 * global variables: opt_num (number of options), c (return value of getopt)
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return Returns EXIT_SUCCESS or EXIT_FAILURE.
 */

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
                fprintf(stderr, "ispalindrom: fopen failed: %s \n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            readFromSource(input);
            fclose(input);
        }
    }
    else
    {
        readFromSource(stdin);
    }

    return EXIT_SUCCESS;
}
