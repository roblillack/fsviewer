#include "df.h"

#include <stdio.h>
#include <sys/statvfs.h>

static const char* formatk(unsigned long long k)
{
    static char buffer[32];

    if (k >= 1000 * 1000)
        snprintf(buffer, sizeof(buffer), "%.4g GB", (double)k / (1000 * 1000));
    else if (k >= 1000)
        snprintf(buffer, sizeof(buffer), "%.4g MB", (double)k / 1000);
    else
        snprintf(buffer, sizeof(buffer), "%llu kB", k);

    return buffer;
}

const char* DiskFree(const char* dir)
{
    struct statvfs st;

    if (!dir || statvfs(dir, &st) != 0)
        return "?";

    return formatk((unsigned long long)st.f_bavail * st.f_frsize / 1000);
}
