//C++ libs
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
//Unix libs
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "builtin.h"

/* CWD_BUFSIZE
 * buffer size for char buffer the holds the current working directory
 */
#define CWD_BUFSIZE 1024

/* MAX_ARGS
 * This is the maximum number of arguments that can be passed via the command 
 * line to quash. in reality qargv should be able to handle an arbitrarily
 * large number of arguments, but for the sake of this argument, we will use
 * a simple constant length array to keep from having to resize the qargv
 * array for different numbers of arguments
 */
#define MAX_ARGS 100

/* PATH_DELIM
 * Delimiter for directories of PATH environment variables
 */
#define PATH_DELIM ':'

/* INTERFACE_[PREFX|SEPARATOR|SUFFIX]
 * These constants make up the title that gets displayed when the user is 
 * prompted for input via the command line. 
 */
#define INTERFACE_PREFIX "quash"
#define INTERFACE_SEPARATOR ":"
#define INTERFACE_SUFFIX ">"

using namespace std;

// pid of the current foreground process
pid_t fpid = -1;

// status of main thread foreground execution
bool fg_exec = false;


/*
 * Signal handler for child processes
 */
void 
handleChildDone(int signal)
{
	pid_t pid;
	int status;

	pid = waitpid(WAIT_ANY, &status, WNOHANG | WUNTRACED);

	if (pid == -1) {
		perror("error");
    	return; 
  	}

	if (pid > 0) {
  	
	  	if (fpid == pid) {
	  		fpid = -1;
	  		fg_exec = false;
	  		return;
	  	}

	  	if (WIFEXITED(status)) {
	  		printf("[%d] Finished\n", pid);
	  		return;
	  	}

		if (WIFSIGNALED(status)) {
			printf("[%d] Terminated (Signal %d)\n", pid, WTERMSIG(status));
			return;
		}

		if (WIFSTOPPED(status)) {
			printf("[%d] Finished (Signal %d)\n", pid, WSTOPSIG(status));
			return;
		}
	}
}

/*-----------------------------------------------
   QUASH UTILITY FUNCTIONS
---------------------------------------------- */

/*
 * Get an environment variable
 */
const char *
qgetenv(map<string, string> *envVars, string name)
{
	return envVars->find(name)->second.c_str();
}

/*
 * Set an environment variable
 */
void
qsetenv(map<string, string> *envVars, char *name, char *value)
{
	(*envVars)[string(name)] = string(value);
}


/*
 * Display command line message to prompt user for input
 */
int 
prompt(string cwd, char *qargv[])
{
	string input, qarg;
	int qargc = 0;

	cout << INTERFACE_PREFIX << INTERFACE_SEPARATOR << cwd << INTERFACE_SUFFIX << ' ';
	getline(cin, input);

	if (input.length() == 0)
		return qargc;

	stringstream ss(input);

	while(ss.good())
	{
		ss >> qarg;
		strcpy(qargv[qargc++], qarg.c_str());
	}
	return qargc;
}

/*
 * Environment variable initalizations for quash
 */
void 
init(map<string, string> *envVars)
{
	envVars->insert(pair<string, string>("PATH", "/bin:/usr/bin"));
	envVars->insert(pair<string, string>("HOME", getenv("HOME")));
	chdir(getenv("HOME"));
}

/* 
 * Deallocate memory and exit program
 */
void
smash(){;}

/*-----------------------------------------------
   MAIN IMPLEMENTATION
---------------------------------------------- */

int 
main(int argc, char *argv[])
{
	map<string, string> envVars;
	char *qargv[MAX_ARGS];
	int qargc;
	char cwd[CWD_BUFSIZE];
	pid_t pid;

	// Set up the quash environment
	init(&envVars);

	// Set up space for input args
	for(int i=0; i<MAX_ARGS; i++)
		qargv[i] = new char[256];

	signal(SIGCHLD, handleChildDone);

	// User input loop
	while(true)
	{
		// Get command
		getcwd(cwd, CWD_BUFSIZE);
		qargc = prompt(cwd, qargv);

		// Run cd
		if (strcmp(qargv[0], "cd") == 0) {
			if (qargc < 2) {
				cd(qgetenv(&envVars, "HOME"));
			} else {
				if (cd(qargv[1]) < 0)
					cout << qargv[1] << ": no such directory" << endl;
			}
		}

		// Run set
		else if (strcmp(qargv[0], "set") == 0) {
			if (qargc > 2) {
				qsetenv(&envVars, qargv[1], qargv[2]);
			} else {
				cout << "usage: set environment_variable value" << endl;
			}
		}

		// Run get
		else if (strcmp(qargv[0], "get") == 0) {
			if (qargc > 1) {
				const char* value = qgetenv(&envVars, qargv[1]);

				if (strcmp(value, qargv[1]) == 0)
					cout << "Item does not exist" << endl;
				else
					cout << qargv[1] << ":" << qgetenv(&envVars, qargv[1]) << endl;
			} else {
				cout << "usage: get environment_variable" << endl;
			}
		}

		// Exit the program
		else if (strcmp(qargv[0], "exit") == 0 || strcmp(qargv[0], "quit") == 0) {
			break;
		}

		// Program Execution
		else if(qargc > 0) {
			pid = fork();
            
			if (pid == 0) {
				
				stringstream ss(qgetenv(&envVars, "PATH"));
				string curPath;
				char cmdbuf[128];

				while(ss.good())
				{
					// get next path
					getline(ss, curPath, PATH_DELIM);

					// zero the cmdbuf
					memset(cmdbuf, 0, 128);
					cmdbuf[0] = '\0';

					// set first param as path to executable
					strcat(cmdbuf, curPath.c_str());
					strcat(cmdbuf, "/");
					strcat(cmdbuf, qargv[0]);

					// replace the last argument with NULL pointer
					// if the last argument is an ampersand
					if (strcmp(qargv[qargc-1], "&") == 0)
						qargv[qargc-1] = NULL;
					else
						qargv[qargc] = NULL;

					// execute
					execv(cmdbuf, qargv);
				}

				// command not found
				perror(qargv[0]);
				exit(-1);
				
			} else {
				
				if (strcmp(qargv[qargc-1], "&") == 0) {
					printf("[%d] Running in background\n", pid);
				} else {
					fpid = pid;
 					fg_exec = true;
					while(fg_exec) pause();
				}
			}
		}
	} 

	//deallocate variables and exit quash
	for(int i=0; i<MAX_ARGS; i++)
		delete [] qargv[i];

	smash();

	return 0;
}
