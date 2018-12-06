/***************************************************************************//**

    @file         main.c

    @author       Stephen Brennan, Shunsuke Haga

    @date         Thursday,  6 December 2018

    @brief        SHSH inspired by LSH (Libstephen SHell)

*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

/*
  Function Declarations for builtin shell commands:
*/
int lsh_cd(char **args);
int lsh_echo(char **args);
int lsh_cat(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_pwd(char **args);
int lsh_sort(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
*/
char *builtin_str[] = {
	"cd",
	"cat",
	"echo",
	"help",
	"exit",
	"pwd",
	"sort"
};

int (*builtin_func[]) (char **) = {
	&lsh_cd,
	&lsh_cat,
	&lsh_echo,
	&lsh_help,
	&lsh_exit,
	&lsh_pwd,
	&lsh_sort
};

int lsh_num_builtins() {
	return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

/**
   @brief Bultin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
*/
int lsh_cd(char **args)
{
	if (args[0] == NULL) {
		fprintf(stderr, "lsh: expected argument to \"cd\"\n");
	} else {
		if (chdir(args[0]) != 0) {
			perror("lsh");
		}
	}
	return 1;
}

/**
   @brief Bultin command: pwd
   @param args List of args.  args[0] is "pwd".
   @return Always returns 1, to continue executing.
*/
#define LSH_PATHSIZE 1024
int lsh_pwd(char **args)
{
	if (args[0] == NULL) {
		printf("\n");
	} else {
		char cwd[LSH_PATHSIZE];
		if (getcwd(cwd, sizeof(cwd)) != NULL)
			printf("%s\n", cwd);
		else
			perror("lsh");
	}
	return 1;
}


/**
   @brief Bultin command: print
   @param args List of args.  args[0] is "echo".  args[1] is the statment.
   @return Always returns 1, to continue executing.
*/
int lsh_echo(char **args)
{
	if (args[0] == NULL) {
		printf("\n");
	} else {
		int i = 1;
		while (args[i] != NULL) {
			if ( args[i][0] == '$')
				printf("%s", getenv(args[i] + 1) );
			else
				printf("%s", ( args[i]));
			if (args[i++ + 1] != NULL)
				printf(" ");
			else
				printf("\n");
		}
	}
	return 1;
}

/**
   @brief Bultin command: print file
   @param args List of args.  args[0] is "cat".  args[1] is the filename.
   @return Always returns 1, to continue executing.
*/
int lsh_cat(char **args) {
	int file;
	char buffer[256];
	int read_size;
	int len = 0;

	// Count number in args
	while (args[len] != NULL)
		len++;

	if (len < 2) {
		fprintf(stderr, "Error: usage: cat filename\n");
		return 1;
	}
	for (int i = 1; i < len; i++) {
		file = open(args[i], O_RDONLY);
		if (file == -1) {
			fprintf(stderr, "Error: %s: file not found\n", args[i]);
			return 1;
		}
		while((read_size = read(file, buffer, 256)) > 0)
			write(1, &buffer, read_size);
		close(file);
	}
	return 1;
}


/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
*/
int lsh_help(char **args)
{
	int i;
	printf("Shunsuke Haga's SHSH\n");
	printf("the forked project from Stephen Brennan's LSH\n");
	printf("Type program names and arguments, and hit enter.\n");
	printf("The following are built in:\n");

	for (i = 0; i < lsh_num_builtins(); i++) {
		printf("  %s\n", builtin_str[i]);
	}

	printf("Use the man command for information on other programs.\n");
	return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
*/
int lsh_exit(char **args)
{
	return 0;
}

/**
   @brief Bultin command: sort
   @param args List of args.  args[0] is "sort".  args[1] is the statment.
   @return Always returns 1, to continue executing.
*/
#define LSH_RL_BUFSIZE 1024
int lsh_sort(char **args)
{
	int i, j;
	char *temp;
	int len = 0;

	if (args[1] == NULL) {
		// If no arguments (passed by pipes)
		int bufsize = LSH_RL_BUFSIZE;
		char *buffer = malloc(sizeof(char) * bufsize), *c;
		int position = 1;

		while (buffer) {
			// Read buffers from stdin
			c = fgets(buffer, bufsize, stdin);
			if (!c){
				// End of the input
				break;
			} else {
				// Store buffer to args.
				buffer[strcspn(buffer, "\n")] = 0;
				args[position++] = strdup(buffer);
			}

			if (position >= bufsize) {
				// If we have exceeded the buffer, reallocate.
				bufsize += LSH_RL_BUFSIZE;
				buffer = realloc(buffer, bufsize);
				if (!buffer) {
					fprintf(stderr, "lsh: allocation error\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}

	// Count number in args
	while (args[len] != NULL)
		len++;
	// Bubble sort
	for (i = 1; i < len ; ++i) {
		for(j = i + 1; j < len; ++j) {
			if(strcmp(args[i], args[j]) > 0) {
				temp = args[i];
				args[i] = args[j];
				args[j] = temp;
			}
		}
	}

	// Print
	for (i = 1; i < len; i++) 
		printf("[%d]: %s\n", i,( args[i]));
	return 1;
}

/**
   @brief Launch a program and wait for it to terminate.
   @param args Null terminated list of arguments (including program).
   @return Always returns 1, to continue execution.
*/
int lsh_launch(char **args) {
	int i = 0;
			
	// Execute inbuild commands
	for (int j = 0; j < lsh_num_builtins(); j++) {
		if (strcmp(args[0], builtin_str[j]) == 0) 
			return (*builtin_func[j])(args);
	}

	// Execute external commands
	pid_t pid; int status;
	pid = fork();
	if (pid == 0) {
		// Child process
		if (execvp(args[0], args) == -1)
			perror("lsh");
		exit(EXIT_FAILURE);
	} else if (pid < 0) {
		// Error creating process
		perror("lsh");
	} else {
		// Parent process
		do 
			waitpid(pid, &status, WUNTRACED);
		while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	return 1; // return 1 to continue
}

/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
*/
int lsh_execute(char **args)
{
	int i;
	pid_t pid;
	int status;

	if (args[0] == NULL) {
		// An empty command was entered.
		return 1;
	}

	int pipe_locate[10], pipe_count = 0;
	pipe_locate[0] = -1;
	int len = 0;
	while (args[len] != NULL)
		len++;

	for (i = 0; args[i] != NULL; i++) {
		// Check for pipes
		if (strcmp(args[i], "|") == 0) {
			pipe_count++;
			pipe_locate[pipe_count] = i;
			args[i] = NULL;
		}
	}

	int pfd[9][2];
	if (pipe_count == 0) {
		// No pipe: execute and return
		pid = fork();
		if (pid == 0) {
			// Child Process
			exit(lsh_launch(args));
		} else {
			// Parent Process
			do {
				waitpid(pid, &status, WUNTRACED);
			} while (!WIFEXITED(status) && !WIFSIGNALED(status));
			return WEXITSTATUS(status);
		}
	}

	for (i = 0; i < pipe_count + 1; i++) {
		if (i != pipe_count)
			// Pipe created
			pipe(pfd[i]);
		if (fork() == 0) {
			// Child Process
			if (i == 0){
				// The first pipe (STDOUT -> PIPE)
				dup2(pfd[0][1], 1);
				close(pfd[0][0]); close(pfd[0][1]);
			} else if (i == pipe_count) {
				// The last pipe (PIPE -> STDIN)
				dup2(pfd[i - 1][0], 0);
				close(pfd[i - 1][0]); close(pfd[i - 1][1]);
			} else {
				// Mid pipe (STDOUT -> PIPE -> STDIN)
				dup2(pfd[i - 1][0], 0);
				dup2(pfd[i][1], 1);
				close(pfd[i - 1][0]); close(pfd[i - 1][1]);
				close(pfd[i][0]); close(pfd[i][1]);
			}
			lsh_launch(args + pipe_locate[i] + 1);
			exit(0);
		} else if (i > 0) {
			// Parent Process
			// Close used pipes
			close(pfd[i - 1][0]); close(pfd[i - 1][1]);
		}
	} 

	for (i = 0; i < pipe_count + 1; i++) {
		wait(&status);
	}
	return 1;
}

#define LSH_RL_BUFSIZE 1024
/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
*/
char *lsh_read_line(void)
{
	int bufsize = LSH_RL_BUFSIZE;
	int position = 0;
	char *buffer = malloc(sizeof(char) * bufsize);
	int c;

	if (!buffer) {
		fprintf(stderr, "lsh: allocation error\n");
		exit(EXIT_FAILURE);
	}

	while (1) {
		// Read a character
		c = getchar();

		if (c == EOF) {
			exit(EXIT_SUCCESS);
		} else if (c == '\n') {
			buffer[position] = '\0';
			return buffer;
		} else {
			buffer[position] = c;
		}
		position++;

		// If we have exceeded the buffer, reallocate.
		if (position >= bufsize) {
			bufsize += LSH_RL_BUFSIZE;
			buffer = realloc(buffer, bufsize);
			if (!buffer) {
				fprintf(stderr, "lsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
	}
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
*/
char **lsh_split_line(char *line)
{
	int bufsize = LSH_TOK_BUFSIZE, position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token, **tokens_backup;

	if (!tokens) {
		fprintf(stderr, "lsh: allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, LSH_TOK_DELIM);
	while (token != NULL) {
		tokens[position] = token;
		position++;

		if (position >= bufsize) {
			bufsize += LSH_TOK_BUFSIZE;
			tokens_backup = tokens;
			tokens = realloc(tokens, bufsize * sizeof(char*));
			if (!tokens) {
				free(tokens_backup);
				fprintf(stderr, "lsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, LSH_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}


/**
   @brief Loop getting input and executing it.
*/
void lsh_loop(void)
{
	char *line;
	char **args;
	int status;

	do {
		printf("shsh!%% ");
		line = lsh_read_line();
		args = lsh_split_line(line);
		status = lsh_execute(args);

		free(line);
		free(args);
	} while (status);
}

/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
*/
int main(int argc, char **argv)
{
	// Load config files, if any.
	setenv("0", "- SHSH", 1);
	setenv("SHELL", "- SHSH", 1);
	// Run command loop.
	lsh_loop();

	// Perform any shutdown/cleanup.
	return EXIT_SUCCESS;
}
