/*Program that acts as a simple shell, taking command line arguments,
 * parsing with an external library, then executing the commands. 
 * Handles IO redirection, piping, and other features.*/
#include <stdio.h>
#include <mush.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pwd.h>

void readNewCommand(FILE*);

int globalPid;

void sigintHandler(int signum) {
    if (globalPid > 0) {
        kill(globalPid, SIGINT);
        printf("\n");
    } else {
        signal(SIGINT, SIG_IGN);
    }
}

/*Closes all pipe file descriptors*/
void closePipes(pipeline line, int pipefds[][2]) {
    int j;
    for (j = 0; j < line->length; j++) {
        close(pipefds[j][0]);
        close(pipefds[j][1]);
    }
}

/*Manually performs the cd function*/
void cdFunction(pipeline line, FILE* file) {
    char* home_dir = getenv("HOME");
    if(home_dir == NULL) {
        struct passwd *pw = getpwuid(getuid());
        if (pw == NULL) {
            fprintf(stderr, "Error: Unable to determine home directory.\n");
            readNewCommand(file);
            return;
        }
        home_dir = pw->pw_dir;
    }
    if(line->stage->argc == 1) {
        if (chdir(home_dir) != 0) {
            perror("chdir");
        }
    } else {
       if(chdir(line->stage->argv[1]) != 0) {
            perror("chdir");
       }
    }
    free_pipeline(line);
    readNewCommand(file);
}

/*Executes a command, dealing with piping and io redirection*/
void execute(pipeline line, int pipefds[][2], FILE* file) {
    pid_t pid;
    int i;
    char* inputFile;
    char* outputFile;
    if(line->length == 1) { 
        pid = fork();

        if(pid < 0) {
            fprintf(stderr, "Fork failure\n");
            free_pipeline(line);
            readNewCommand(file);
            return;
        } else if(pid == 0) {
            if((inputFile = line->stage->inname) != NULL) {
                int inputfd = open(inputFile, O_RDONLY);
                if (inputfd == -1) {
                    perror("open");
                    close(inputfd);
                    free_pipeline(line); 
                    readNewCommand(file);
                    return;
                }
                if(dup2(inputfd, STDIN_FILENO) == -1) {
                    perror("dup2");
                    close(inputfd);
                    free_pipeline(line);
                    readNewCommand(file);
                    return;
                }
                close(inputfd);
            }
            if((outputFile = line->stage->outname) != NULL) {
                int outputfd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (outputfd == -1) {
                    perror("open");
                    close(outputfd);
                    readNewCommand(file);
                    return;
                }
                if(dup2(outputfd, STDOUT_FILENO) == -1) {
                    perror("dup2");
                    close(outputfd);
                    free_pipeline(line); 
                    readNewCommand(file);
                }
                close(outputfd);
            }
            execvp(line->stage->argv[0], line->stage->argv);
        } else {
            int status;
            int childPid;
            globalPid = pid;
            childPid = wait(&status);
            if (WEXITSTATUS(status) != 0) {
                printf("Process %d exited with an error value.\n", childPid);
            }
            globalPid = 0;
        } 
    } else if(line->length > 1) {
        for(i = 0; i < line->length; i++) {
            pid = fork();
            
            if(pid < 0) {
                fprintf(stderr, "Fork failure\n");
                closePipes(line, pipefds);
                break;
            } else if(pid == 0) {    
                if(i > 0) {
                    if(dup2(pipefds[i-1][0], STDIN_FILENO) == -1) {
                        perror("dup2");
                        closePipes(line, pipefds);
                        break;
                    }
                } else if((inputFile = line->stage->inname) != NULL) {
                    int inputfd = open(inputFile, O_RDONLY);
                    if (inputfd == -1) {
                        perror("open");
                        closePipes(line, pipefds);
                        break;
                    }
                    if(dup2(inputfd, STDIN_FILENO) == -1) {
                        perror("dup2");
                        closePipes(line, pipefds);
                        break;
                    }
                    close(inputfd);
                } if(i < line->length-1) {
                    if(dup2(pipefds[i][1], STDOUT_FILENO) == -1) {
                        perror("dup2");
                        closePipes(line, pipefds);
                        break;
                    }
                } else if ((outputFile = line->stage[line->length-1].outname) != NULL) {
                    int outputfd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    if (outputfd == -1) {
                        perror("open");
                        closePipes(line, pipefds);
                        break;
                    }
                    if(dup2(outputfd, STDOUT_FILENO) == -1) {
                        perror("dup2");
                        close(outputfd);
                        closePipes(line, pipefds);
                        break;
                    }
                    close(outputfd);
                }
                closePipes(line, pipefds);     
                execvp(line->stage[i].argv[0], line->stage[i].argv);
            } else {
                int status;
                int childPid;
                globalPid = pid;
                childPid = wait(&status);
                if (WEXITSTATUS(status) != 0) {
                    printf("Process %d exited with an error value.\n", childPid);
                }
                if (i > 0) {
                    close(pipefds[i-1][0]);
                }
                if (i < line->length - 1) {
                    close(pipefds[i][1]);
                };
                globalPid = 0;
            } 
        }
    }
    free_pipeline(line);
    readNewCommand(file);
}

/*Prints the prompt and checks command line for piping, or cd*/
void readNewCommand(FILE* file) {
    char* commandLine;
    int i;
    
    pipeline line;
    if(file == stdin) {
        printf("8-P ");
        commandLine = readLongString(file);
    } else {
        commandLine = readLongString(file);
        if(commandLine == NULL) {
            free(commandLine);
            return;
        }
        printf("8-P %s\n", commandLine);
    }
    if(commandLine == NULL) {
        free(commandLine);
        return;
    }
    line = crack_pipeline(commandLine);
    
    if(line == NULL) {
        free(commandLine);
        readNewCommand(file);
        return;
    }
    int pipefds[line->length][2];

    if(!strncmp(line->cline, "cd ", 3)) {
        cdFunction(line, file);
    } else if(line->length > 1) {
        for (i = 0; i < line->length; i++) {
            if (pipe(pipefds[i]) == -1) {
                perror("pipeline");
                free(commandLine);
                free_pipeline(line);
                readNewCommand(file);
                return;
            }
        }
        free(commandLine);
        execute(line, pipefds, file);
    } else {
        free(commandLine);
        execute(line, NULL, file);
    }
}

/*Main function that calls helper*/
int main(int argc, char* argv[]) {
    struct sigaction sa;
    sa.sa_handler = sigintHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    if(argc == 1) {
        readNewCommand(stdin);
    } else if(argc == 2) {
        FILE *file = fopen(argv[1], "r");
        if (file == NULL) {
            perror("fopen");
            return 1;
        }
        readNewCommand(file);
        fclose(file);
    }
    return 1;
}
