
#include <cstdio>

#if _POSIX_C_SOURCE >= 1 or _XOPEN_SOURCE or _POSIX_SOURCE or _BSD_SOURCE or _SVID_SOURCE
#define PUTCHAR putchar_unlocked
#define FGETS   fgets_unlocked
#else
#define PUTCHAR putchar
#define FGETS   fgets
#endif

/**
 * File based string acquisition.
 * May be redefined for more accurate measurements.
 */
#define FOREACH_LINE for(const char * s; (s = FGETS(buffer, MAX_SIZE, stdin)) != NULL; /* Empty */)

const size_t MAX_SIZE = 4096;
static char buffer[MAX_SIZE];
