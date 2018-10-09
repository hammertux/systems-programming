#include <stdio.h>

int mystrlen(char* input_string) {
    int char_counter = 0;

    while(*input_string != '\0') {
        input_string++;
        char_counter++;
    }

    return char_counter;
}


int mystrcmp(char* first_string, char* second_string) {
    //If two strings are not of same length, they are already different.
    int first_length = mystrlen(first_string);
    int second_length = mystrlen(second_string);

    if(first_length != second_length) {
        return 0;
    }

    while(*first_string != '\0') {
        first_string++;
        second_string++;
        if(*first_string != *second_string) {
            return 0;
        }
    }

    return 1;
}


int main(int argc, char** argv) {
    int same = 0;
    if(argc != 3) {
        printf("Usage: mystrcmp <first_input_string_with_no_space_inside_it> <second_input_string_with_no_space_inside_it>\n\n");
        return 1;
    }

    same = mystrcmp(argv[1], argv[2]);

    if(same) {
        printf("The two strings are identical.\n");
    }
    else {
        printf("The two strings are different.\n");
    }

    return 0;
}