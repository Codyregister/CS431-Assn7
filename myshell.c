/******************************************************************************************
/ PROGRAM:     myshell.c
/ AUTHOR:      Cody Register
/ DESCRIPTION: Acts as a very simple command line interpreter.  It reads commands from 
/              standard input entered from the terminal and executes them. The shell does
/              not include any provisions for control structures, pipes, redirection, 
/              background processes, environmental variables, or other advanced properties
/              of a modern shell. All commands are implemented internally and do not rely
/              on external system programs.
/
/              (c) Regis University 
 ********************************************************************************************/
#include <pwd.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_PATH_LENGTH           256
#define MAX_BUFFER_LENGTH         256
#define MAX_FILENAME_LENGTH       256

char buffer[MAX_BUFFER_LENGTH] = {0};
char filename[MAX_FILENAME_LENGTH] = {0};
unsigned int result;

// Implements various UNIX commands using POSIX system calls
int do_cat(const char* filename);
int do_cd(char* dirname);
int do_ls(const char* dirname);
int do_mkdir(const char* dirname);
int do_pwd(void);
int do_rm(const char* filename);
int do_rmdir(const char* dirname);
int do_stat(char* filename);
int execute_command(char* buffer);

// Remove extraneous whitespace at the end of a command to avoid parsing problems

void strip_trailing_whitespace(char* string) {
    int i = strnlen(string, MAX_BUFFER_LENGTH) - 1;

    while (isspace(string[i]))
        string[i--] = 0;
}

// Display a command prompt including the current working directory

void display_prompt(void) {
    char current_dir[MAX_PATH_LENGTH];

    if (getcwd(current_dir, sizeof (current_dir)) != NULL)
        fprintf(stdout, "%s>", current_dir);
}

int main(int argc, char** argv) {
    while (1) {
        display_prompt();

        // Read a line representing a command to execute from standard input into a character array
        if (fgets(buffer, MAX_BUFFER_LENGTH, stdin) != 0) {
            strip_trailing_whitespace(buffer); // Clean up sloppy user input
            memset(filename, 0, MAX_FILENAME_LENGTH); //Reset filename buffer after each command execution

            // As in most shells, "cd" and "exit" are special cases that needed to be handled separately
            if ((sscanf(buffer, "cd %s", filename) == 1) || (!strncmp(buffer, "cd", MAX_BUFFER_LENGTH))) {
                result = do_cd(filename);
                continue;
            } else if (!strncmp(buffer, "exit", MAX_BUFFER_LENGTH)) {
                exit(0);
            } else {
                execute_command(buffer);
            }
        }
    }

    return 0;
}

// changes the current working directory

int do_cd(char* dirname) {
    struct passwd *p = getpwuid(getuid());
    int result;

    if (strnlen(dirname, MAX_PATH_LENGTH) == 0)
        strncpy(dirname, p->pw_dir, MAX_PATH_LENGTH);

    result = chdir(dirname);
    if (result < 0)
        fprintf(stderr, "cd: %s\n", strerror(errno));

    return result;
}

// lists the contents of a directory

int do_ls(const char* dirname) {
    DIR* d;
    struct dirent* entry;
    struct stat info;

    d = opendir(dirname);
    if (d == NULL) {
        fprintf(stderr, "Could not open directory %s: %s\n", dirname, strerror(errno));
        return EXIT_FAILURE;
    }

    chdir(dirname);

    entry = readdir(d);

    while (entry != NULL) {
        if (stat(entry->d_name, &info) < 0) {
            fprintf(stderr, "stat: %s: %s\n", entry->d_name, strerror(errno));
        }
        if (S_ISDIR(info.st_mode)) {
            fprintf(stdout, "%-30s\t<dir>\n", entry->d_name);
        } else {
            fprintf(stdout, "%-30s\n", entry->d_name);
        }
        entry = readdir(d);

    }
    closedir(d);
}

// outputs the contents of a single ordinary file

int do_cat(const char* filename) {
    char buffer[2048];
    int inFile = open(filename, O_RDONLY, 0);
    int readCount;
    if (inFile < 0) {
        printf("Unable to open %s: %s\n", filename, strerror(errno));
        return -1;
    }
    do {
        readCount = read(inFile, buffer, sizeof (buffer));
        write(1, buffer, sizeof (buffer));

    } while (readCount > 0);

    close(inFile);

}

// creates a new directory 

int do_mkdir(const char* dirname) {
    if (mkdir(dirname, 0755) < 0) {
        fprintf(stderr, "Error making directory %s: %s\n", dirname, strerror(errno));
    }

}

// removes a directory as long as it is empty

int do_rmdir(const char* dirname) {
    if (rmdir(dirname) < 0) {
        fprintf(stderr, "Error removing directory %s: %s\n", dirname, strerror(errno));
    }
}

// outputs the current working directory

int do_pwd(void) {
    char current_dir[MAX_PATH_LENGTH];

    if (getcwd(current_dir, sizeof (current_dir)) != NULL)
        fprintf(stdout, "%s>", current_dir);
}

// removes (unlinks) a file

int do_rm(const char* filename) {
    if (unlink(filename) < 0) {
        fprintf(stderr, "Error unlinking file %s: %s\n", filename, strerror(errno));

    }
}

// outputs information about a file

int do_stat(char* filename) {
    struct stat info;

    if (stat(filename, &info) < 0) {
        fprintf(stderr, "Error getting stats for %s: %s\n", filename, strerror(errno));
    } else {
        fprintf(stdout, "File Name: %s\n", filename);
        fprintf(stdout, "Total Size: %i\n", (int) info.st_size);
        fprintf(stdout, "Last Modified: %s", ctime(&info.st_mtime));
        fprintf(stdout, "Protection: %i\n", (int) info.st_mode);
        fprintf(stdout, "Number of hardlinks: %i\n", (int) info.st_nlink);
        fprintf(stdout, "Inode: %i\n", (int) info.st_ino);
    }
}

int execute_command(char* buffer) {
    if (sscanf(buffer, "cat %s", filename) == 1) {
        result = do_cat(filename);
        return result;
    }
    if (sscanf(buffer, "stat %s", filename) == 1) {
        result = do_stat(filename);
        return result;
    }
    if (sscanf(buffer, "mkdir %s", filename) == 1) {
        result = do_mkdir(filename);
        return result;
    }
    if (sscanf(buffer, "rmdir %s", filename) == 1) {
        result = do_rmdir(filename);
        return result;
    }
    if (sscanf(buffer, "rm %s", filename) == 1) {
        result = do_rm(filename);
        return result;
    } else if ((sscanf(buffer, "ls %s", filename) == 1) || (!strncmp(buffer, "ls", MAX_BUFFER_LENGTH))) {
        if (strnlen(filename, MAX_BUFFER_LENGTH) == 0)
            sprintf(filename, ".");

        result = do_ls(filename);
        return result;
    } else if (!strncmp(buffer, "pwd", MAX_BUFFER_LENGTH)) {
        result = do_pwd();
        return result;
    } else // Invalid command
    {
        if (strnlen(buffer, MAX_BUFFER_LENGTH) != 0)
            fprintf(stderr, "myshell: %s: No such file or directory\n", buffer);
        return -1;
    }
}
