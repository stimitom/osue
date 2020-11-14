#include <stdio.h>
#include <string.h>

void isPalindrom(char str[]) {
    int l = 0; 
    int h = strlen(str) -1; 


    while (h > l) {
        if (str[l++] != str[h--])
        {
            /* Compares characters from beginning and end to the middle. Returns if they are not equal.*/
            printf("%s is not a palindrom.\n", str); 
            return;
        }
    }
    printf("%s is a palindrom.\n", str);
}

int main (int argc, char *argv[]){
    isPalindrom("Abba"); 
    isPalindrom("abba");
    isPalindrom("lagerregal");
    isPalindrom("anna abba anna");
    return 0;
}

