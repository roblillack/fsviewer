#include <stdio.h>

void
regerror(char* s)
{
    fprintf(stderr, "regexp(3): %s", s);
}
