/*------------------------------------------------
        This code belongs to Kushagra Gupta
        The University of Texas at Dallas
--------------------------------------------------
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <bits/types/FILE.h>
#include <stdbool.h>
#include <fcntl.h>

/*******************GLOBAL DEC.********************/

#define ROW 10
#define COL 100
char pth[ROW][COL]= {"/bin/"};//Path variable for storing search paths

int pos; bool has;

char error_message[30]="An error has occured \n"; // Error message

/*******************BUILTINS**********************/

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

int dash_cd(char **args){ //cd builtin
    if (args[1] == NULL){
        if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
            perror("Error writing");
        }
    } else {
        if (chdir(args[1]) != 0){ //using chdir system call
            if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
                perror("Error writing");
            }
        }
    }
    return 1;
}

int dash_exit(char **args){ // exit builtin
    exit(0);
}

int dash_path(char **args){ // function for storing search paths entered by user
    if (args[1] == NULL){
        if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
            perror("Error writing");
        }
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

/*****************PROCESS CREATION*****************************/

int dash_process_creation(char **args) {

    pid_t pid; int status; int i = 0;

        pid = fork();

        if (pid == 0) {
            if (has == true) {  // if user wants output redirected to a file

                char *filename = args[pos + 1];

                int fd;
                fpos_t position;
                fgetpos(stdout, &position);

                if ((fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
                    if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)) {
                        perror("Error writing");
                    }
                }

                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);
                fflush(stdout);

                args[pos] = NULL;
                args[pos + 1] = NULL;

                while (i < sizeof(pth) / sizeof(char *)) {
                    strcat(pth[i], args[0]);
                    if (access(pth[i], X_OK) == 0) {
                        if (execv(pth[i], args) < 0) {
                            if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)) {
                                perror("Error writing");
                            }
                            exit(1);
                        }
                    } else {
                        i++;
                    }
                }

                close(fd);
                clearerr(stdout);
                fsetpos(stdout, &position);  //restoring console position

            } else {     // using stdout
                while (i < sizeof(pth) / sizeof(char *)) {
                    strcat(pth[i], args[0]);
                    if (access(pth[i], X_OK) == 0) { // access system call to check for existence
                        if (execv(pth[i], args) < 0) {
                            if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)) {
                                perror("Error writing");
                            }
                            exit(1);
                        }
                    } else {
                        i++;
                    }
                }
            }
        } else if (pid < 0) {
            if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)) {
                perror("Error writing");
            }
            exit(1);
        } else {
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status));
        }
    return 1;
}


/*******************EXECUTING COMMANDS******************/

int execute(char **args){ //if argc[1]=builtin then builtin otherwise launch cmd
    int i;
    if (args[0] == NULL){
        return 1;
    }

    for (i = 0; i < dash_num_builtins(); i++) {
        if (strcmp(args[0],built_in[i]) == 0){
            return (*builtin_funct[i])(args);
        }
    }
    return dash_process_creation(args);
}

/********************READ COMMAND***********************/

char *read_command(void){ //function to read input
    char *input = NULL;
    size_t bufsize = 0; size_t characters;
    characters = getline(&input, &bufsize, stdin);
    if (characters == -1){
        if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
            perror("Error writing");
        }
    }
    return input;
}

/**********************************************PARSE INPUT******************************************/

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
        exit(1);
    }

    token = strtok(input, SEPARATORS); //tokenizing input

    while (token != NULL) {
        cmd[index] = token;

        if (strcmp(token,">") == 0){
            pos=index;
            has=true;
            index++;
        } else if (strcmp(token,"&") == 0){
            cmd[index]=NULL;
            execute(cmd); //executing parallel processes
            //free(cmd);
            index=0;
        } else {
            index++;
        }

        if (index > bufsize) {
            bufsize += BUF_SIZE;
            cmd = realloc(cmd, bufsize * sizeof(char *));
            if (!cmd) {
                if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
                    perror("Error writing");
                }
                exit(1);
            }
        }

        token = strtok(NULL, SEPARATORS);
    }
    cmd[index] = NULL;
    return cmd;
}




/******************LOOPING***********************/

void dash_looping(){
    char *input;
    char **args;
    int status;

    do {
        printf("dash> "); //prints dash> constantly in interactive mode
        input = read_command();
        args = parse(input);
        status = execute(args);

        free(input);
        free(args);
    } while (status);
}

/****************MAIN***************************/

int
main(int argc, char **argv){
    if (argc > 2) {
        if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
            perror("Error writing");
        }
    } else if (argc == 1) {
        dash_looping();
    } else { //batch mode
        char *input; char **args; int status; size_t bufsize = 0; size_t len;

        FILE *fp = fopen(argv[argc-1], "r+");

        if(!fp){
            if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
                perror("Error writing");
            }
            exit(1);
        }

        do {
            len = getline(&input, &bufsize, fp);
            if (len == -1){
                if (write(STDERR_FILENO, error_message, strlen(error_message)) != strlen(error_message)){
                    perror("Error writing");
                }
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

/**************************END******************************/


