// Name: Daniel Saraf, ID: 311312045
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <stdlib.h>
#include <pwd.h>
#include <errno.h>
#include <stdbool.h>

#define TRUE 1
#define FALSE 0

int isCdCommand(char *command);

int checkBGRunning(char *command);

void executeCommand(char *command);

int countWords(char *command);

void getPath(char *command, char *path);

void cleanSpaces(char *command);

void insertCommand(char command[101], char commandHistory[100][102], int pid, int *commCount);

int isAlive(int pid);

int getCommandPID(char *cmd);

void getArgs(char *command, char *arg);

void readCommand(char *command);

void exitCommand();

int cdCommand(char *command, char *lastPath);

void handleChild(char *command, int commCount, char commandHistory[100][102]);

void convertWordSpaces(char *str, int mode);

int cdToHome(char *arg);

void makeLegalCommand(char *command);

int checkIfEcho(char *command);

int checkAllSpaces(char *command);


int main() {
    setbuf(stdout,NULL);
    char command[101];
    char lastPath[200];
    getcwd(lastPath, 200);
    int pid, commCount = 0, bgRun;
    char commandHistory[100][102];
    while (TRUE) {
        readCommand(command);
        makeLegalCommand(command);
        //printf("%s\n",command);
        if (strlen(command) == 0) continue;
        //check if command is in background-running mode
        bgRun = checkBGRunning(command);
        // check if exit command
        if (!strcmp(command, "exit") || !strcmp(command, "exit &")) {
            exitCommand();
        }
            // check if cd command
        else if (isCdCommand(command)) {
            insertCommand(command, commandHistory, getpid(), &commCount);
            if (!cdCommand(command, lastPath))
                return 0;
            continue;
        } else if ((pid = fork()) == 0) {
            handleChild(command, commCount, commandHistory);
            return 0;
        } else if (pid > 0) {
            // parent process
            insertCommand(command, commandHistory, pid, &commCount);
            signal(SIGCHLD, SIG_IGN);
            if (!bgRun)
                waitpid(pid, NULL, 0);
            else
                // wait 0.5 sec to increase the chances that child pid will print before prompt - looks more aesthetic
                sleep(0.5);

        } else {
            fprintf(stderr, "Error : fork failed\n");
        }
    }
}

void handleChild(char *command, int commCount, char commandHistory[100][102]) {
    // child process
    int i;
    if (!strcmp(command, "jobs")) { // jobs command
        for (i = 0; i < commCount; i++) {
            if (isAlive(getCommandPID(commandHistory[i])))
                printf("%s\n", commandHistory[i]);
        }
    } else if (!strcmp(command, "history")) { // history command
        for (i = 0; i < commCount; i++) {
            printf("%s ", commandHistory[i]);
            if (isAlive(getCommandPID(commandHistory[i])) || i == commCount)
                printf("RUNNING\n");
            else
                printf("DONE\n");
        }
        printf("%d history RUNNING\n", getpid());
    } else { // system call
        printf("%d\n", getpid());
        executeCommand(command);
    }
}

int cdCommand(char *command, char *lastPath) {
    // cd command doesnt create a child process and not support background running
    printf("%d\n", getpid());
    if (countWords(command) == 1) {
        getcwd(lastPath, 200);
        if (!cdToHome("cd"))
            return FALSE;
    } else if (countWords(command) == 2) {
        char arg[99] = {};
        getArgs(command, arg);
        if (!strcmp(arg, "..")) {
            getcwd(lastPath, 200);
            if (chdir("..") != 0) {
                fprintf(stderr, "Error: cd .. command failed\n");
                return FALSE;
            }
        } else if (!strcmp(arg, "-")) {
            char current[200];
            getcwd(current, 200);
            if (chdir(lastPath) != 0) {
                fprintf(stderr, "Error: cd - command failed\n");
                return FALSE;
            }
            strcpy(lastPath, current);
        } else if (!strcmp(arg, "~")) {
            getcwd(lastPath, 200);
            if (!cdToHome(arg))
                return FALSE;
        } else {
            getcwd(lastPath, 200);
            char *temp = arg;
            if (arg[0] == '~') {
                if (!cdToHome(arg))
                    return FALSE;
                temp = arg + 2;
            }
            if (chdir(temp) != 0) {
                fprintf(stderr, "Error: No such file or directory\n");
                return FALSE;
            }
        }
    } else {
        fprintf(stderr, "Error: Too many arguments\n");
    }
    return TRUE;
}

// remove unwonted spaces
void makeLegalCommand(char *command) {
    if (checkAllSpaces(command)) {
        strcpy(command, "");
        return;
    }
    char copy[101];
    int i = 0, j = 0, n = strlen(command);
    int echoCommand = checkIfEcho(command);
    copy[0] = 0;
    while (command[i] == 32)
        i++;
    for (; i < n;) {
        while (command[i] != 32 && command[i] != 0) {
            if (echoCommand && command[i] == 34) { // echo command & found " - dont remove spaces until next "
                copy[j++] = command[i++];
                while (command[i] != 0)
                    if (command[i] != 34)
                        copy[j++] = command[i++];
                    else if (command[i - 1] == 92)
                        copy[j++] = command[i++];
                    else
                        break;

                copy[j++] = command[i++];
            } else
                copy[j++] = command[i++];
        }
        copy[j++] = 32;
        while (command[i] == 32) {
            i++;
        }
    }
    if (j != 0)
        copy[--j] = 0;
    strcpy(command, copy);
}

int checkAllSpaces(char *command) {
    int i = 0;
    while (command[i] != 0) {
        if (command[i] != 32)
            return FALSE;
        i++;
    }
    return TRUE;
}

int checkIfEcho(char *command) {
    char commandCopy[101];
    strcpy(commandCopy, command);
    char *ptr = strtok(commandCopy, " ");
    if (!strcmp(ptr, "echo")) {
        return TRUE;
    }
    return FALSE;
}

int cdToHome(char *arg) {
    const char *homedir;
    if ((homedir = getenv("HOME")) == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }
    if (chdir(homedir) != 0) {
        fprintf(stderr, "Error: %s command failed\n", arg);
        return FALSE;
    }
    return TRUE;
}

void exitCommand() {
    printf("%d\n", getpid());
    exit(0);
}

void readCommand(char *command) {
    printf("> ");
    fgets(command, 100, stdin);
    command[strlen(command) - 1] = 0; // clear '\n'
}

void getArgs(char *command, char *arg) {
    char copy[101];
    strcpy(copy, command);
    char *ptr = strtok(copy, " ");
    ptr = strtok(NULL, " ");
    strcpy(arg, ptr);
}

int isAlive(int pid) {
    if (kill(pid, 0) < 0) {
        if (errno == ESRCH) {
            return 0;
        }
    }
    if (getppid() == pid) {
        return 0;
    }
    return 1;
}

int getCommandPID(char *cmd) {
    char copy[101];
    strcpy(copy, cmd);
    char *ptr = strtok(copy, " ");
    return atoi(ptr);
}

void insertCommand(char command[101], char commandHistory[100][102], int pid, int *commCount) {
    // insert command to history list
    char commandPlusPID[110] = {};
    char pidString[10];
    sprintf(pidString, "%d", pid);
    strcat(commandPlusPID, pidString);
    strcat(commandPlusPID, " ");
    if (command[strlen(command) - 1] == '&')
        command[strlen(command) - 2] = 0;
    strcat(commandPlusPID, command);
    strcpy(commandHistory[(*commCount)++], commandPlusPID);
}

void executeCommand(char *command) {
    if (checkBGRunning(command))
        command[strlen(command) - 1] = 0;
    char commandCopy[101];
    int echoCommand = FALSE;
    char path[20];
    strcpy(commandCopy, command);
    int numbOfWords = countWords(commandCopy), i = 0;
    char *argv[101];
    char *ptr;
    if (checkIfEcho(command)) {
        // convert all the spaces in the words inside "" to special char to parse it later
        convertWordSpaces(command, 0);
        strcpy(commandCopy, command);
        ptr = strtok(commandCopy, " ");
        while (ptr != NULL && strcmp(ptr, "&")) {
            convertWordSpaces(ptr, 1);
            argv[i++] = ptr;
            ptr = strtok(NULL, " ");
        }
        argv[i] = NULL;
    } else {
        ptr = strtok(commandCopy, " ");
        while (ptr != NULL && strcmp(ptr, "&")) {
            argv[i++] = ptr;
            ptr = strtok(NULL, " ");
        }
        argv[i] = NULL;
    }
    // save the path into 'path'
    getPath(command, path);
    if (execvp(path, argv) == -1) {
        fprintf(stderr, "Error in system call\n");
    }
}

void convertWordSpaces(char *str, int mode) {
    int i, j, n = strlen(str), found = 0;
    char temp[101];
    if (mode == 0) { // convert spaces to special char
        for (i = j = 0; i < n; i++) {
            if (found == 0) {
                if (str[i] == 34) {
                    temp[j++] = str[i];
                    found = 1;
                } else
                    temp[j++] = str[i];
            } else {
                if (str[i] == 34) {
                    temp[j++] = str[i];
                    found = 0;
                } else {
                    if (str[i] == 32)
                        temp[j++] = 1;
                    else
                        temp[j++] = str[i];
                }
            }
        }

    } else { // convert special char to spaces
        for (i = j = 0; i < n; i++) {
            if (str[i] == 34 || str[i] == 92) {
                if (i > 0)
                    if (str[i - 1] == 92)
                        temp[j++] = str[i];
            } else if (str[i] == 1) temp[j++] = 32;
            else temp[j++] = str[i];
        }
    }
    temp[j] = 0;
    strcpy(str, temp);
}

void getPath(char *command, char *path) {
    char *commandName;
    path[0] = 0;
    if (countWords(command) > 1) {
        commandName = strtok(command, " ");
        strcat(path, commandName);
    } else {
        char commandCopy[101];
        strcpy(commandCopy, command);
        cleanSpaces(commandCopy);
        strcat(path, commandCopy);
    }
}

void cleanSpaces(char *str) {
    // To keep track of non-space character count
    int count = 0, i;

    // Traverse the given string. If current character
    // is not space, then place it at index 'count++'
    for (i = 0; str[i]; i++)
        if (str[i] != ' ')
            str[count++] = str[i]; // here count is
    // incremented
    str[count] = '\0';
}

int countWords(char *str) {
    int state = 0;
    unsigned wc = 0; // word count
    if (checkBGRunning(str)) // ignore '&' sign
        wc = -1;
    // Scan all characters one by one
    while (*str) {
        // If next character is a separator, set the
        // state as OUT
        if (*str == ' ' || *str == '\n' || *str == '\t')
            state = 0;
            // If next character is not a word separator and
            // state is OUT, then set the state as IN and
            // increment word count
        else if (state == 0) {
            state = 1;
            ++wc;
        }
        // Move to next character
        ++str;
    }
    return wc;
}

int checkBGRunning(char *command) {
    if (command[strlen(command) - 1] == '&')
        return TRUE;
    else
        return FALSE;
}

int isCdCommand(char *command) {
    if (!strcmp(command, "cd")) return TRUE;
    int i;
    char prefix[4];
    for (i = 0; i < 3; i++)
        prefix[i] = command[i];
    prefix[3] = 0;
    if (!strcmp(prefix, "cd "))
        return TRUE;
    else
        return FALSE;
}