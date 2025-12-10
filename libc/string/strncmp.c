#include <string.h>

int strncmp(char s1[], char s2[], int n) {
    int i;
    for (i = 0; s1[i] == s2[i]; i++) {
        if (i >= n) return 0;
    }
    return s1[i] - s2[i];
}

