#include <stdio.h>
#include <stdlib.h>


int main()
{
    extern char **environ;
    for(int i = 0; environ[i] != NULL; i++)
    {
        printf("environ[%2d]: %s\n",i,environ[i]);
    }
    // char *path = getenv("PATH");
    // printf("path: %s\n",path);
    /* puts */
    // puts("hello");
    // puts("goodmoring");
    /* fputs */
    // fputs("hello\n",stdout);
    // fputs("good",stdout);
    // fputs("moning\n",stdout);
    exit(0);
}