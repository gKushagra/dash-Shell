/*
        This code belongs to Kushagra Gupta
        The University of Texas at Dallas

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <bits/types/FILE.h>
#include <stdbool.h>
#include <fcntl.h>

/*****************************GLOBAL DEC.*****************************************/

#define ROW 10
#define COL 100
char pth[ROW][COL]= {"/bin/"};

int pos; bool has;

char error_message[30]="An error has occured \n";

/*******************************BUILTINS******************************************/

int dash_cd(char **args);
int dash_exit(char **args);
int dash_path(char **args);

char *built_in[]={
        "cd","exit","path"
};

int (*builtin_funct[])(char **)={
        &dash_cd,
        &dash_exit,
        &dash_path
};

int dash_num_builtins(){
    return sizeof(built_in)/ sizeof(char *);
}

int dash_cd(char **args){
    if (args[1] == NULL){
        if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
            perror("Error writing");
        }
        // fprintf(stderr, "Argument expected to cd \n");
    } else {
        if (chdir(args[1]) != 0){
            if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
                perror("Error writing");
            }
            //write(STDERR_FILENO, error_message, strlen(error_message));
            // perror("Sorry! No such directory");
        }
    }
    return 1;
}

int dash_exit(char **args){
    return 0;
}

int dash_path(char **args){
    if (args[1] == NULL){
        if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
            perror("Error writing");
        }

        //write(STDERR_FILENO, error_message, strlen(error_message));
        //fprintf(stderr,"Argument Expected to path \n");
    } else {
            int i=1, j=0;
            while (i < ROW){
                if (args[i] != NULL) {
                    strcpy(pth[j], args[i]);
                    printf("Path added to Search Path %s\n",pth[j]);
                    i++;
                    j++;
                } else {
                    break;
                }
            }
        }
    return 1;
}

/****************************PROCESS CREATION*************************************/

int dash_process_creation(char **args) {

    pid_t pid; int status; int i = 0;

    pid = fork();

    if (pid == 0) {
        if (has == true) {

            char *filename=args[pos+1];

            fpos_t position;
            fgetpos(stdout, &position);
            int fd;

            if ((fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
                if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
                    perror("Error writing");
                }

                // write(STDERR_FILENO, error_message, strlen(error_message));
                //perror("Error opening file");
            }

            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);

            close(fd);

            args[pos] = NULL;
            args[pos+1] = NULL;

            while (i < sizeof(pth)/ sizeof(char *)) {
                strcat(pth[i], args[0]);
                if (access(pth[i], X_OK) == 0) {
                    if (execv(pth[i], args) < 0) {
                        if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
                            perror("Error writing");
                        }

                        // write(STDERR_FILENO, error_message, strlen(error_message));
                        //perror("No such command!");
                        exit(1);
                    }
                    //exit(0);
                } else {
                    i++;
                }
            }

            clearerr(stdout);
            fsetpos(stdout, &position);

        } else {
            while (i < sizeof(pth)/ sizeof(char *)) {
                strcat(pth[i], args[0]);
                if (access(pth[i], X_OK) == 0) {
                    if (execv(pth[i], args) < 0) {
                        if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
                            perror("Error writing");
                        }

                        // write(STDERR_FILENO, error_message, strlen(error_message));
                        //perror("No such command!");
                        exit(1);
                    }
                    //exit(1);
                } else {
                    i++;
                }
            }
        }
    } else if (pid < 0){
        if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
            perror("Error writing");
        }

        // write(STDERR_FILENO, error_message, strlen(error_message));
        //perror("Error in forking");
        exit(1);
    } else {
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status));
    }
    return 1;
}


/***************************EXECUTING COMMANDS***********************************/

int execute(char **args){
    int i;
    if (args[0] == NULL){
        return 1;
    }

    for (i = 0; i < dash_num_builtins(); ++i) {
        if (strcmp(args[0],built_in[i]) == 0){
            return (*builtin_funct[i])(args);
        }
    }
    return dash_process_creation(args);
}

/*************************************READ COMMAND*********************************/

char *read_command(void){
    char *input = NULL;
    size_t bufsize = 0; size_t characters;
    characters = getline(&input, &bufsize, stdin);
    if (characters == -1){
        if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
            perror("Error writing");
        }

        // write(STDERR_FILENO, error_message, strlen(error_message));
        //perror("Failed to read line");
    }
    return input;
}

/************************************PARSE INPUT**********************************/

#define BUF_SIZE 64
#define SEPARATORS "' ','\t','\r','\n','\a'"
char **parse(char *input) {
    int bufsize = BUF_SIZE, index = 0;
    char **cmd = malloc(bufsize * sizeof(char *));
    char *token;

    if (!cmd) {
        if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
            perror("Error writing");
        }

        // write(STDERR_FILENO, error_message, strlen(error_message));
        //fprintf(stderr, "Error in allocating memory");
        exit(1);
    }

    token = strtok(input, SEPARATORS);

    while (token != NULL) {
        cmd[index] = token;

        if (strcmp(token,"&") == 0){
            cmd[index]=NULL;
            execute(cmd);
            index=0;
        }
        else if (strcmp(token,">") == 0){
            pos=index;
            has=true;
            index++;
        }
        else {
            index++;
        }

        if (index > bufsize) {
            bufsize += BUF_SIZE;
            cmd = realloc(cmd, bufsize * sizeof(char *));
            if (!cmd) {
                if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
                    perror("Error writing");
                }

                // write(STDERR_FILENO, error_message, strlen(error_message));
                //fprintf(stderr, "Error in allocating memory");
                exit(1);
            }
        }

        token = strtok(NULL, SEPARATORS);
    }
    cmd[index] = NULL;
    return cmd;
}




/******************************LOOPING*******************************************/

void dash_looping(){
    char *input;
    char **args;
    int status;

    do {
        printf("dash> ");
        input = read_command();
        args = parse(input);
        status = execute(args);

        free(input);
        free(args);
    } while (status);
}

/*******************************MAIN*********************************************/

int
main(int argc, char **argv){
    if (argc > 2) {
        if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
            perror("Error writing");
        }

        // write(STDERR_FILENO, error_message, strlen(error_message));
        //perror("More than one arguments present!");
    } else if (argc == 1) {
        dash_looping();
    } else {
        char *input; char **args; int status; size_t bufsize = 0; size_t len;

        FILE *fp = fopen(argv[argc-1], "r+");

        do {
            len = getline(&input, &bufsize, fp);
            if (len == -1){
                if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
                    perror("Error writing");
                }

                // write(STDERR_FILENO, error_message, strlen(error_message));
                // perror("Done with this file!");
                exit(1);
            }
            args = parse(input);
            status = execute(args);

            free(input);
            free(args);
        } while (status);

        fclose(fp);
    }
    return 0;
}

/********************************************************************************/


