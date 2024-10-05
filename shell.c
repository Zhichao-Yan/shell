#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define MAX_ARGS 1024
#define MAX_INPUT 1024
#define PATH_MAX 1024

/* parse command line string and return argument numbers */
static int parseline(char *buf, char **argv)
{
    char *token = NULL;
    int i = 0;
    token = strtok(buf," ");
    while(token != NULL)
    {
        argv[i++] = token;
        token = strtok(NULL," ");
    }
    argv[i] = NULL;
    return i;
}

int main()
{
    printf("|==================|\n");
    printf("|    LINUX SHELL   |\n");
    printf("|==================|\n");
    /* shell prompt */
    char *prompt = getenv("USER");
    if(prompt == NULL)
    {
        prompt = "root";
    }
    char hostname[1024];
    gethostname(hostname, sizeof(hostname));
    /* shell command history file */
    FILE *history_file = fopen(".shell_history", "a");
    fclose(history_file);

    while(1)
    {
        /* make command line prompt */
        char cwd[PATH_MAX] = {'\0'};
        getcwd(cwd, sizeof(cwd));
        printf("\n%s@%s:%s$ ", prompt, hostname,cwd);

        /* Store fgets content at input or ctrl+D */
        char input[MAX_INPUT] = {'\0'};
        if (fgets(input, MAX_INPUT, stdin) == NULL)
        {
            printf("\nexiting\n");
            exit(0);
        }
        /* Remove '\n' from string */
        input[strcspn(input, "\n")] = '\0';
        
        // Open .history write at end mode
        FILE *history_file = fopen(".shell_history", "a");
        fprintf(history_file, "%s\n", input);
        fclose(history_file);

        char buf[MAX_INPUT];
        strcpy(buf,input);
        char *argv[MAX_ARGS];
        int argc = parseline(buf,argv);

        /* FOREGROUND AND BACKGROUND */
        int bg = 0;
        if (argc > 0) // Check if last argument is "&"
        {
            if(strcmp(argv[argc - 1], "&") == 0)
            {
                argv[argc - 1] = NULL; // Remove "&" from args
                argc--;     // self decrease
                if(argc == 0)
                {
                    printf("syntax error near unexpected token \'&\'");
                    continue;
                }
                bg = 1;     // Set background flag
            }else
            {
                int k = strlen(argv[argc - 1]);
                if(argv[argc - 1][k - 1] == '&')
                {
                    argv[argc - 1][k - 1] = '\0';
                    bg = 1;
                }
            }
        }

        /* input is a blank line  */
        if(argv[0] == NULL) 
            continue;

        /* exit command */
        if (strcmp(argv[0], "exit") == 0)
        {
            printf("exit\n");
            exit(0);
        }

        /* history */
        if(strcmp(argv[0], "history") == 0)
        {
            FILE *history_file = fopen(".shell_history", "r");
            char line[MAX_INPUT] = {'\0'};
            while(fgets(line,MAX_INPUT,history_file))
            {
                printf("%s", line);
            }
            fclose(history_file);
            continue;
        }

        if (strcmp(argv[0], "cd") == 0 && argv[1] != NULL)
        {
            if (chdir(argv[1]) == -1)
            {
                printf("cd: %s: No such file or directory\n", argv[1]);
            }
            else
            {
                setenv("PWD",argv[1],1);
            }
            continue;
        }
    }
    exit(0);
}