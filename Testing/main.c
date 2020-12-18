#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

char *pgmName;
char *total;
static int hexCharToInt(char character);
static void fromIntToHexChar(int i, char *c);
static char *reverseString(char *str);
// static char *addHexStrings(char *a, char *b);
static void addHexStrings(char *a, char *b);
// static void addHexStrings(char *a, char *b, char *endResult);


int main(int argc, char *argv[])
{
    pgmName = argv[0];
 
    if ((total = malloc(1)) == NULL)
    {
        fprintf(stderr, "Malloc falied\n");
    }
    addHexStrings("AFAFAF","AFAFA11984312983" );
    printf("%s\n", total);
}

static int hexCharToInt(char character)
{

    if (character >= '0' && character <= '9')
    {

        return character - '0';
    }
    if (character >= 'A' && character <= 'F')
    {

        return character - 'A' + 10;
    }
    if (character >= 'a' && character <= 'f')
    {
        return character - 'a' + 10;
    }
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


static void addHexStrings(char *a, char *b)
{
    
    if (strlen(a) > strlen(b)){
        addHexStrings(b,a);
        return;
    }
    // {
    //     second = malloc(strlen(a));
    //     strncpy(second, a, strlen(a));
    //     first = malloc(strlen(b));
    //     strncpy(first, b, strlen(b));
    // }
    // else
    // {
    //     second = malloc(strlen(b));
    //     second = strncpy(second, b, strlen(b));
    //     first = malloc(strlen(a));
    //     first = strncpy(first, a, strlen(a));
    // }
    char first[strlen(a)];
    strncpy(first, a, strlen(a));
    char second[strlen(b)];
    strncpy(second, b, strlen(b));

    int n1 = strlen(first), n2 = strlen(second);
    int diff = n2 - n1;
    int resultLen = 0;

    // Take an empty string for storing result
    char result[(n1 + n2)];
    memset(result, 0, (n1 + n2 - 1));

    // Initially take carry zero
    // char *carry = malloc(1);
    char carry[1];
    carry[0] = '0';

    char temp[1];

    // Traverse from end of both strings
    for (int i = n1 - 1; i >= 0; i--)
    {
        int sum = hexCharToInt(first[i]) + hexCharToInt(second[i + diff]) + hexCharToInt(carry[0]);
        fromIntToHexChar((sum % 16), temp);
        strncat(result, temp, 1);
        fromIntToHexChar((sum / 16), carry);
        resultLen++;
    }

    // Add remaining digits of str2[]
    char temp2[1];

    for (int i = n2 - n1 - 1; i >= 0; i--)
    {
        int sum = hexCharToInt(second[i]) + hexCharToInt(carry[0]);

        fromIntToHexChar((sum % 16), temp2);

        strncat(result, temp2, 1);
        fromIntToHexChar((sum / 16), carry);
        resultLen++;
    }

    // Add remaining carry
    if (carry[0] != '0')
    {
        strcat(result, carry);
        resultLen++;
    }

    
    if (realloc(total,resultLen) == NULL)
    {
        // exitError("Realloc for endresult failed.", errno);
        fprintf(stderr, "Failed realloc: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    strncpy(total, reverseString(result), resultLen);
}



// static void addHexStrings(char *a, char *b, char *endResult)
// {
//     char *first;
//     char *second;
//     if (strlen(a) > strlen(b))
//     {
//         second = malloc(strlen(a));
//         strncpy(second, a, strlen(a));
//         first = malloc(strlen(b));
//         strncpy(first, b, strlen(b));
//     }
//     else
//     {
//         second = malloc(strlen(b));
//         second = strncpy(second, b, strlen(b));
//         first = malloc(strlen(a));
//         first = strncpy(first, a, strlen(a));
//     }

//     int n1 = strlen(first), n2 = strlen(second);
//     int diff = n2 - n1;
//     int resultLen = 0;

//     // Take an empty string for storing result
//     char result[(n1 + n2)];
//     memset(result, 0, (n1 + n2 - 1));

//     // Initially take carry zero
//     // char *carry = malloc(1);
//     char carry[1];
//     carry[0] = '0';

//     char temp[1];

//     // Traverse from end of both strings
//     for (int i = n1 - 1; i >= 0; i--)
//     {
//         int sum = hexCharToInt(first[i]) + hexCharToInt(second[i + diff]) + hexCharToInt(carry[0]);
//         fromIntToHexChar((sum % 16), temp);
//         strncat(result, temp, 1);
//         fromIntToHexChar((sum / 16), carry);
//         resultLen++;
//     }

//     // Add remaining digits of str2[]
//     char temp2[1];

//     for (int i = n2 - n1 - 1; i >= 0; i--)
//     {
//         int sum = hexCharToInt(second[i]) + hexCharToInt(carry[0]);

//         fromIntToHexChar((sum % 16), temp2);

//         strncat(result, temp2, 1);
//         fromIntToHexChar((sum / 16), carry);
//         resultLen++;
//     }

//     // Add remaining carry
//     if (carry[0] != '0')
//     {
//         strcat(result, carry);
//         resultLen++;
//     }

//     if (realloc(endResult, resultLen - 1) == NULL)
//     {
//         // exitError("Realloc for endresult failed.", errno);
//         fprintf(stderr, "Failed realloc: %s\n", strerror(errno));
//         exit(EXIT_FAILURE);
//     }

//     strncpy(endResult, reverseString(result), resultLen);

//     free(second);
//     free(first);
// }

// static char *addHexStrings(char *a, char *b)
// {
//    char *first;
//    char *second;
//     // Before proceeding further, make sure length
//     // of str2 is larger.
//     if (strlen(a) > strlen(b))
//     {
//         second =  malloc(strlen(a));
//         strncpy(second, a, strlen(a));
//         first =  malloc(strlen(b));
//         strncpy(first, b, strlen(b));
//     }else{
//         second =  malloc(strlen(b));
//         strncpy(second, b, strlen(b));
//         first =  malloc(strlen(a));
//         strncpy(first, a, strlen(a));
//     }

//     int n1 = strlen(first), n2 = strlen(second);
//     int diff = n2 - n1;
//     int resultLen = 0;

//     // Take an empty string for storing result
//     char result[(n1 + n2)];
//     memset(result, 0, (n1 + n2));

//     // Initially take carry zero
//     char *carry = malloc(1);
//     carry[0] = '0';

//     char *temp;

//     temp = malloc(1);
//     // ERror handling

//     // Traverse from end of both strings
//     for (int i = n1 - 1; i >= 0; i--)
//     {
//         int sum = hexCharToInt(first[i]) + hexCharToInt(second[i + diff]) + hexCharToInt(carry[0]);
//         fromIntToHexChar((sum % 16), temp);
//         strncat(result, temp, 1);
//         fromIntToHexChar((sum / 16), carry);
//         resultLen++;
//     }

//     // Add remaining digits of str2[]
//     char *temp2;
//     temp2 = malloc(1);
//     //Error handling

//     for (int i = n2 - n1 - 1; i >= 0; i--)
//     {
//         int sum = hexCharToInt(second[i]) + hexCharToInt(carry[0]);

//         fromIntToHexChar((sum % 16), temp2);

//         strncat(result, temp2, 1);
//         fromIntToHexChar((sum / 16), carry);
//         resultLen++;
//     }

//     // Add remaining carry
//     if (carry[0])
//     {
//         strcat(result, carry);
//         resultLen++;
//     }
//     printf("resultlen : %d\n", resultLen);
//     printf("Resut: %s\n", reverseString(result));
//     // reverse resultant string
//     return reverseString(result);
// }
