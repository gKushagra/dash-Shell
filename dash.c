#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <bits/types/FILE.h>

#define ROW 10
#define COL 100
char pth[ROW][COL]= {"/bin/"};


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
        fprintf(stderr, "Argument expected to cd \n");
    } else {
        if (chdir(args[1]) != 0){
            perror("Sorry! No such directory");
        }
    }
    return 1;
}

int dash_exit(char **args){
    return 0;
}

int dash_path(char **args){
    if (args[1] == NULL){
        fprintf(stderr,"Argument Expected to path \n");
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

/*************************************READ COMMAND*********************************/

char *read_command(void){
    char *input = NULL;
    size_t bufsize = 0; size_t characters;
    characters = getline(&input, &bufsize, stdin);
    if (characters == -1){
        perror("Failed to read line");
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
        fprintf(stderr, "Error in allocating memory");
        exit(EXIT_FAILURE);
    }

    token = strtok(input, SEPARATORS);
    while (token != NULL) {
        cmd[index] = token;
        index++;

        if (index > bufsize) {
            bufsize += BUF_SIZE;
            cmd = realloc(cmd, bufsize * sizeof(char *));
            if (!cmd) {
                fprintf(stderr, "Error in allocating memory");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, SEPARATORS);
    }
    cmd[index] = NULL;
    return cmd;
}

/****************************PROCESS CREATION*************************************/

int dash_process_creation(char **args) {

    pid_t pid; int status; int i = 0;

    pid = fork();

    if (pid == 0) {
        while (i < ROW) {
            strcat(pth[i], args[0]);
            if (access(pth[i], X_OK) == 0) {
                if (execv(pth[i], args) == -1) {
                    perror("No such command!");
                }
                //exit(EXIT_FAILURE);
                //i++;
            } else {
                i++;//break;
            }
        }
    } else if (pid < 0){
        perror("Error in forking");
    } else {
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
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
        perror("More than one arguments present!");
    } else if (argc == 1) {
        dash_looping();
    } else {
        char *input; char **args; int status; size_t bufsize = 0; size_t len;

        FILE *fp = fopen(argv[argc-1], "r+");

        len = getline(&input, &bufsize, fp);

        if (len == -1){
            perror("Failed to read line");
        }
        do {
            len = getline(&input, &bufsize, fp);
            args = parse(input);
            status = execute(args);

            free(input);
            free(args);
        } while (status);

        fclose(fp);
    }
    return EXIT_SUCCESS;
}

/********************************************************************************/


