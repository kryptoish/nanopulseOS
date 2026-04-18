#include <string.h>

static int to_upper(int c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int diff = to_upper((unsigned char)*s1) - to_upper((unsigned char)*s2);
        if (diff != 0) return diff;
        s1++;
        s2++;
    }
    return to_upper((unsigned char)*s1) - to_upper((unsigned char)*s2);
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && *s2) {
        int diff = to_upper((unsigned char)*s1) - to_upper((unsigned char)*s2);
        if (diff != 0) return diff;
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return to_upper((unsigned char)*s1) - to_upper((unsigned char)*s2);
}
