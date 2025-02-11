// The MIT License (MIT)
//
// Copyright (c) 2024 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <signal.h>
#include <fcntl.h>

#define WHITESPACE " \t\n" // We want to split our command line up into tokens
                           // so we need to define what delimits our tokens.
                           // In this case white space
                           // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255 // The maximum command-line size

#define MAX_NUM_ARGUMENTS 32

// function for printing out the error message
void error()
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

int main(int argc, char *argv[])
{
    char *command_string = (char *)malloc(MAX_COMMAND_SIZE);
    FILE *file;
    file = stdin;
    int num = 4;
    char *search[] = {"/bin", "/usr/bin", "/usr/local/bin", "./"};

    // opening the file and in batch mode.
    if (argc == 2)
    {

        file = fopen(argv[1], "r");
        if (file == NULL)
        {
            error();
            exit(1);
        }
    }
    // if more than 2 arguments passed
    if (argc > 2)
    {
        error();
        exit(1);
    }
    while (1)
    {
        // interactive mode
        if (argc == 1)
        {
            // Print out the msh prompt
            printf("msh> ");
        }

        // Read the command from the commandi line.  The
        // maximum command that will be read is MAX_COMMAND_SIZE
        // This while command will wait here until the user
        // inputs something.
        while (!fgets(command_string, MAX_COMMAND_SIZE, file))
        {
            // if reaches the end of file, it exits
            if (feof(file))
            {
                exit(0);
            }
        }

        /* Parse input */
        char *token[MAX_NUM_ARGUMENTS];

        // clear the contents of the token array
        memset(token, 0, sizeof(token));

        int token_count = 0;

        // Pointer to point to the token
        // parsed by strsep
        char *argument_pointer;

        char *working_string = strdup(command_string);
        if (working_string == NULL)
        {
            error();
            continue;
        }
        // we are going to move the working_string pointer so
        // keep track of its original value so we can deallocate
        // the correct amount at the end

        char *head_ptr = working_string;

        // Tokenize the input with whitespace used as the delimiter
        while (((argument_pointer = strsep(&working_string, WHITESPACE)) != NULL) &&
               (token_count < MAX_NUM_ARGUMENTS))
        {
            token[token_count] = strndup(argument_pointer, MAX_COMMAND_SIZE);
            if (strlen(token[token_count]) == 0)
            {
                token[token_count] = NULL;
            }
            else
                token_count++;
        }
        token_count++;

        if (token[0] == NULL)
            continue;

        // Built in function cd
        else if (strcmp("cd", token[0]) == 0)
        {
            if (chdir(token[1]) == -1)
            {
                error();
            }
            continue;
        }

        // if user types exit
        else if (strcmp("exit", token[0]) == 0)
        {
            if (token_count <= 2)
            {
                exit(0);
            }
            error();
        }

        else
        {

            char path[MAX_COMMAND_SIZE];
            int dir = 0;

            //checking directory
            for (int i = 0; i < num && !dir; ++i)
            {
                snprintf(path, sizeof(path), "%s/%s", search[i], token[0]);
                // Check if the command is executable in the current directory
                dir = (access(path, X_OK) == 0);
            }

            pid_t pid = fork();

            //checks if child process
            if (pid == 0 && dir)
            {

                for (int i = 0; i < MAX_NUM_ARGUMENTS; i++)
                {
                    if (token[i] == NULL)
                    {
                        break;
                    }

                    if (strcmp(token[i], ">") == 0)
                    {
                        if (token[i + 1] == NULL || token[i + 2] != NULL)
                        {
                            error();
                            exit(0);
                        }

                        int output_fd = open(token[i + 1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                        if (output_fd < 0)
                        {
                            error();
                            exit(0);
                        }

                        //copying output to stdout_fileno
                        dup2(output_fd, STDOUT_FILENO);
                        close(output_fd);

                        token[i] = NULL;
                        break;
                    }
                }
                //executing the command
                execvp(token[0], token);
            }

            if (pid < 0)
            {
                error();
                exit(0);
            }
            else if (pid > 0)
            {
                int status;
                wait(&status);
            }
            else
            {
                error();
                exit(0);
            }
        }
        free(head_ptr);
    }
    free(command_string);
    return 0;
}