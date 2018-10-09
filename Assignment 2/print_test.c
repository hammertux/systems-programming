#include <stdio.h>

void printAlphabet() {
    char c;
    int i;
    for(c = 'A'; c <= 'Z'; c++) {
        for(i = 0; i <= 500000; i++) {
            printf("%c ", c);
        }
    }
}

int main(int argc, char const *argv[])
{
    printAlphabet();
    return 0;
}
