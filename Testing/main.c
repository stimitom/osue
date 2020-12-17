#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

char *pgmName;

static void exitError(char *message, int errnum);
static int hexCharToInt(char character);
static void fromIntToHexChar(int i, char *c);
static char *reverseString(char *str);
static char *addHexStrings(char *first, char *second);

int main(int argc, char *argv[])
{
    pgmName = argv[0];
    char *x = malloc(25);
    x = addHexStrings("AFAF", "100");
}

static int hexCharToInt(char character)
{
    if (character >= '0' && character <= '9')
        return character - '0';
    if (character >= 'A' && character <= 'F')
        return character - 'A' + 10;
    if (character >= 'a' && character <= 'f')
        return character - 'a' + 10;
    return -1;
}

static void fromIntToHexChar(int i, char *c)
{
    sprintf(c, "%X", i);
}

static char *reverseString(char *str)
{

    if (!str || !*str)
    {
        return str;
    }

    int i = strlen(str) - 1, j = 0;

    char c;
    while (i > j)
    {
        c = str[i];
        str[i] = str[j];
        str[j] = c;
        i--;
        j++;
    }
    return str;
}

static char *addHexStrings(char *first, char *second)
{
    // Before proceeding further, make sure length
    // of str2 is larger.
    if (strlen(first) > strlen(second))
    {
        return addHexStrings(second, first);
    }

    int n1 = strlen(first), n2 = strlen(second);
    int diff = n2 - n1;

    // Take an empty string for storing result
    char result[(n1 + n2)];
    memset(result, 0, (n1 + n2));

    int resultLen = 0;

    // Initially take carry zero
    char carry = '\0';

    char *temp;
    if ((temp = malloc(1)) == NULL)
    {
        exitError("Malloc for temp failed.", errno);
    }

    // Traverse from end of both strings
    for (int i = n1 - 1; i >= 0; i--)
    {
        int sum = hexCharToInt(first[i]) + hexCharToInt(second[i + diff]) + hexCharToInt(carry);
        fromIntToHexChar((sum % 16), temp);
        strncat(result, temp, 1);
        fromIntToHexChar((sum / 16), &carry);
    }

    // Add remaining digits of str2[]
    char *temp2;
    if ((temp2 = malloc(1)) == NULL)
    {
        exitError("Malloc for temp2 failed.", errno);
    }
    for (int i = n2 - n1 - 1; i >= 0; i--)
    {
        int sum = hexCharToInt(second[i]) + hexCharToInt(carry);

        fromIntToHexChar((sum % 16), temp2);
        strncat(temp2, '0', 1);
        strncat(result, temp2, 2);
        // carry = fromIntToHexChar (sum / 16);
    }

    // Add remaining carry
    // if (carry)
    // {
    //     strncat(carry, '0', 1);
    //     strcat(result, carry);
    // }

    // reverse resultant string
    return reverseString(result);
}

static void exitError(char *message, int errnum)
{
    fprintf("error\n %s, %d", message, errnum);
}