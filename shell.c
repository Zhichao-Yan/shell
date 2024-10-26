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
    /* shell user */
    char *prompt = getenv("USER");
    if(prompt == NULL)
    {
        prompt = "root";
    }
    /* shell hostname */
    char hostname[1024];
    gethostname(hostname, sizeof(hostname));

    /* shell command history file */
    FILE *history_file = fopen(".shell_history", "a");
    fclose(history_file);

    while(1)
    {
        /* shell cwd */
        char cwd[PATH_MAX] = {'\0'};
        getcwd(cwd, sizeof(cwd));
        /* command line prompt */
        printf("\n%s@%s:%s$ ", prompt, hostname,cwd);

        /* Store fgets content at input or ctrl+D */
        char input[MAX_INPUT] = {'\0'};
        if (fgets(input, MAX_INPUT, stdin) == NULL)
        {
            printf("\nexiting\n");
            exit(0);
        }

        /* record input into .shell_history */
        FILE *history_file = fopen(".shell_history", "a");
        fprintf(history_file, "%s", input);
        fclose(history_file);

        /* remove '\n' from string */
        input[strcspn(input, "\n")] = '\0';

        /* copy buf from input  */
        char buf[MAX_INPUT];
        strcpy(buf,input);

        /* parse buf into arguments */
        char *argv[MAX_ARGS];
        int argc = parseline(buf,argv);

        /* The initial value of MYPATH is imported from PATH */
        // char *mypath = getenv("MYPATH");
        // if (mypath == NULL)
        // {
        //     char *path = getenv("PATH");
        //     if (path != NULL)
        //     {
        //         setenv("MYPATH", path, 1);
        //         mypath = getenv("MYPATH");
        //     }
        // }

        /* export */
        if (strcmp(argv[0], "export") == 0 && argv[1] != NULL)
        {
            char buf[100] = {'\0'};
            strcpy(buf,argv[1]);
            char *ptr = strchr(buf,'=');
            if( ptr != NULL)
            {   
                *ptr = '\0';
                ptr = ptr + 1;
                char *ptr1 = strchr(ptr,':');
                if(ptr1 != NULL)
                {
                    *ptr1 = '\0';
                    ptr1 = ptr1 + 1;
                    char path[1024] = {'\0'};
                    if(*ptr1 == '$')
                    {
                        ptr1 = ptr1 + 1;
                        char *env = getenv(ptr1);
                        if(env)
                        {   
                            sprintf(path,"%s:%s",ptr,env);
                        }else{
                            strcpy(path,ptr);
                        }
                    }else if(*ptr == '$')
                    {
                        ptr = ptr + 1;
                        char *env = getenv(ptr);
                        if(env)
                        {
                            sprintf(path,"%s:%s",env,ptr1);
                        }else{
                            strcpy(path,ptr1);
                        }
                    }else{
                        strcpy(path,ptr);
                    }
                    setenv(buf,path,1);
                }else
                {
                    setenv(buf,ptr,1);
                }
            }
            continue;
        }

        /* foreground and background */
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

        /* ---------- blank input  ---------- */
        if(argv[0] == NULL) 
            continue;

        /* ---------- exit  ---------- */
        if (strcmp(argv[0], "exit") == 0)
        {
            printf("exit\n");
            exit(0);
        }

        /* ---------- history  ---------- */
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

        /* ---------- cd  ----------  */
        if (strcmp(argv[0], "cd") == 0 && argv[1] != NULL)
        {
            if (chdir(argv[1]) == -1)
            {
                printf("cd: %s: No such file or directory\n", argv[1]);
            }else{
                /* if success,the set the environment variable PWD */
                setenv("PWD",argv[1],1);
            }
            continue;
        }

        /* ---------- pwd  ---------- */
        if(strcmp(argv[0], "pwd") == 0)
        {
            getcwd(cwd, sizeof(cwd));
            printf("%s",cwd);
            continue;
        }
        /* ---------- env  ---------- */
        if(strcmp(argv[0], "env") == 0)
        {
            extern char **environ;
            for(int i = 0; environ[i] != NULL; i++)
            {
                printf("environ[%2d]: %s\n",i,environ[i]);
            }
            continue;
        }

        /* ---------- echo  ---------- */
        
    }
    exit(0);
}