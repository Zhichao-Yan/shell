#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#define MAX_ARGS 1024
#define MAX_INPUT 1024
#define MAX_JOBS 20
#define PATH_MAX 1024
#define Done 0
#define Running 1
#define Stopped 2

typedef struct
{
    pid_t pid;
    int counter;
    int status;
    char input[MAX_INPUT];
}job;
job job_list[MAX_JOBS];
int job_controller = 0;     // Quantity of process paused and back-grounded
pid_t process_id;

/* jobs() show the job_list*/
static void jobs()
{
    for (int i = 0; i < MAX_JOBS; i++)
    {
        if (job_list[i].pid > 0)
        {
            printf("[%d] ", job_list[i].counter);
            if (job_list[i].status == Running)
            {
                printf("Running ");
            }
            else if (job_list[i].status == Stopped)
            {
                printf("Stopped ");
            }
            else if (job_list[i].status == Done)
            {
                printf("Done ");
            }
            printf("%s\n", job_list[i].input);
        }
    }
}

/* add job to job_list */
static int add_job(pid_t pid, char *input, int status)
{
    int job_id = 0;
    for (int i = 0; i < MAX_JOBS; i++)
    {
        if (job_list[i].pid == 0) // Find an empty space at list
        {
            job_list[i].pid = pid;
            job_id = ++job_controller;
            job_list[i].counter = job_id;
            strcpy(job_list[i].input, input);
            job_list[i].status = status;
            break;
        }
    }
    return job_id;
}


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

/* parse redirect sign('>' or '<' or ">>") from buf and confirm redirect file */
static int redirect(char *buf,char *type,char *file)
{
    char *ptr = strstr(buf,type);
    if(ptr != NULL)
    {
        size_t len = strlen(type);
        memset (ptr,' ',len);
        ptr = ptr + len;
        while (*ptr == ' ') 
        {
            ++ptr;
        }
        if(*ptr != '\0')
        {
            int i = 0;
            i = strcspn(ptr," ");
            strncpy(file,ptr,i);
            memset (ptr,' ',i);
            return 1;
        }else{
            printf("-bash: syntax error near %s\n",type);
            return -1;
        }
    }
    return 0;
}


/* 
 *  when press ctrl+Z,trigger this 
 *  send SIGSTOP to  foreground job
 */
void signal_tstp_handler(int sig)
{
    if (process_id > 0) {
        kill(process_id, SIGSTOP);
    }
    return;
}

/*  when background job finished
 *  handle the SIGCHLD signal
 */
void signal_child_handler(int sig)
{
    int status;
    pid_t pid;
    if((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        for (int i = 0; i < MAX_JOBS; i++)
        {
            if (job_list[i].pid == pid)
            {
                if (WIFEXITED(status))
                {
                    job_list[i].status = Done; // Update jobs status to DONE
                }
                break;
            }
        }
    }
}

int main()
{
    printf("|==================|\n");
    printf("|    LINUX SHELL   |\n");
    printf("|==================|\n");
    /* signal handle */
    signal(SIGINT, SIG_IGN);
    signal(SIGCHLD, signal_child_handler);
    signal(SIGTSTP, signal_tstp_handler);

    /* shell user */
    char *prompt = getenv("USER");
    if(prompt == NULL)
    {
        prompt = "root";
    }
    /* shell hostname */
    char hostname[PATH_MAX];
    gethostname(hostname, sizeof(hostname));

    /* shell command history file */
    FILE *history_file = fopen(".shell_history", "a");
    fclose(history_file);

    /* the executable program in PATH directories */
    // char command[MAX_INPUT];

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

        char input_file[MAX_INPUT] = {'\0'};
        if(redirect(buf,"<",input_file) < 0)
            continue;
        char output_file[MAX_INPUT] = {'\0'};
        if(redirect(buf,">",output_file) < 0)
            continue;       
        char error_file[MAX_INPUT] = {'\0'};
        if(redirect(buf,"2>",error_file) < 0)
            continue;  

        /* parse buf into arguments */
        char *argv[MAX_ARGS];
        int argc = parseline(buf,argv);

        /* if argv[0] is NULL */
        if(argv[0] == NULL)
        {
            continue;
        }

        /* jobs */
        if (strcmp(argv[0], "jobs") == 0)
        {
            jobs();
            continue;
        }
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
        /* ---------- kill ----------- */
        if (strcmp(argv[0], "kill") == 0)
        {

        }

        /* ---------- echo  ---------- */
        if (strcmp(argv[0], "echo") == 0 && argv[1] != NULL)
        {
            if (argv[1][0] == '$') // Environment variable case
            {
                char *var_name = &argv[1][1];
                char *var_value = getenv(var_name); // Value
                if(var_value != NULL)
                {
                    printf("%s\n", var_value);
                }else{
                    printf("Environment variable not found\n");
                }
                continue;
            }else
            {
                for (int i = 1; argv[i] != NULL; i++)
                {
                    printf("%s ", argv[i]);
                }
                continue;
            }
        }

        // char *path = getenv("PATH");
        // if(path != NULL && strcmp(path, "") != 0)
        // {
        //     char path_copy[PATH_MAX];
        //     strcpy(path_copy,path);
        //     /* parse the path variable into directories */
        //     char *directory[MAX_ARGS];
        //     int k = 0;
        //     char *dir = strtok(path_copy, ":");
        //     while(dir != NULL)
        //     {
        //         directory[k++] = dir;
        //         dir = strtok(NULL, ":");
        //     }
        //     directory[k] = NULL;
        //     /* find command through directories*/
        //     int found = 0;
        //     for(int i = 0; directory[i] != NULL && !found; i++)
        //     {
                
        //         sprintf(command, "%s/%s", directory[i], argv[0]);
        //         if(access(command,X_OK) == 0)
        //         {
        //             found = 1;
        //             break;
        //         }
        //     }
        //     if (!found)
        //     {
        //         printf("%s: command not found\n",argv[0]);
        //         continue;
        //     }
        // }

        process_id = fork();            // Process Identifier
        if (process_id < 0)             // Error creating child process
        {
            printf("create process error\n");
            exit(1);
        }else if(process_id == 0)       // children process
        {
            if(strcmp(input_file,"") != 0)
            {
                int fd = open(input_file, O_RDONLY);
                if (fd == -1)
                {
                    perror("Error opening input file");
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if(strcmp(output_file,"") != 0)
            {
                int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if (fd == -1)
                {
                    perror("Error opening output file");
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);                
            }
            if(strcmp(error_file,"") != 0)
            {
                int fd = open(error_file, O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if (fd == -1)
                {
                    perror("Error opening error file");
                    exit(1);
                }
                dup2(fd, STDERR_FILENO);
                close(fd);                 
            }
            if (execvp(argv[0], argv) < 0)
            {
                printf("Error executing command\n");
                exit(1);
            }
        }else if(process_id > 0)    // father process
        {
            if(bg == 0)             // process running in foreground
            {
                int status;
                waitpid(process_id, &status, WUNTRACED);
                if(WIFSTOPPED(status))
                {
                    int job_id = add_job(process_id,input,Stopped);
                    printf("\n[%d] Stopped %s",job_id,input);
                }
            }else{                  // process running in background
                int job_id = add_job(process_id,input,Running);
                printf("[%d] %d",job_id,process_id);
            }
        }
    }
    exit(0);
}