#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* formatk(unsigned long k)
{
    static char buffer[10];

    if (k >= 1000 * 1000)
        sprintf(buffer, "%.4g GB", (double)k / (1000 * 1000));
    else if (k >= 1000)
        sprintf(buffer, "%.4g MB", (double)k / 1000);
    else
        sprintf(buffer, "%ld kB", k);

    return buffer;
}

const char* DiskFree(const char* dir)
{
    unsigned long total = 0; /* Total Space */
    char buffer[1024];
    snprintf(buffer, 1024, "BLOCKSIZE=1000 df %s", dir);

    FILE* f = popen(buffer, "r");

    if (!f) {
        fprintf(stderr, "%s %d: Can't run df, %s\n", __FILE__, __LINE__, strerror(errno));
        return "?";
    }

    /* Read in from the pipe until we hit the end */
    for (;;) {
        int n = 0; /* number of words */
        char* p = NULL;
        char* fs = NULL; /* File System */
        char* word[10]; /* pointer to each word */
        // char  buffer[1024];

        /* Read in line by line */
        if (!fgets(buffer, 1024, f))
            break;

        /* Strip the whitespace and break up the line */
        for (p = buffer; n < 10;) {
            // skip leading whitespace:
            while (*p && isspace(*p))
                p++;
            if (!*p)
                break;
            // skip over the word:
            word[n++] = p;

            while (*p && !isspace(*p))
                p++;
            if (!*p)
                break;
            *p++ = 0;
        }
        // for (int i = 0; i < n; i++)
        // {
        //     printf("%d: %s\n", i, word[i]);
        // }

        /* Get the file system name */
        fs = strdup(word[0]);
        /* And make sure it is local */
        if (strncmp(fs, "/dev", 4))
            continue;

        /* ok we found a line with a /dev at the start */
        /* Total */
        total += strtol(word[3], 0, 10);

        if (fs)
            free(fs);
    }
    pclose(f);

    return formatk(total);
}