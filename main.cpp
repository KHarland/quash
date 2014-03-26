#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "builtin.h"
#include "job.h"

using namespace std;

/* CWD_BUFSIZE
 * buffer size for char buffer the holds the current working directory
 * 
 * MAX_ARGS
 * This is the maximum number of arguments that can be passed via the command 
 * line to quash. in reality argv should be able to handle an arbitrarily
 * large number of arguments, but for the sake of this argument, we will use
 * a simple constant length array to keep from having to resize the argv
 * array for different numbers of arguments
 * 
 * MAX_JOBS
 * Maxmimum number of jobs that can be run concurrently
 * 
 * PATH_DELIM
 * Delimiter for directories of PATH environment variables
 * 
 * ENV_DELIM 
 * Delimiter for environment variables
 * 
 * INTERFACE_[PREFX|SEPARATOR|SUFFIX]
 * These constants make up the title that gets displayed when the user is 
 * prompted for input via the command line. 
 */
#define CWD_BUFSIZE 1024
#define MAX_ARGS 100
#define MAX_JOBS 100
#define PATH_DELIM ':'
#define ENV_DELIM '='
#define INTERFACE_PREFIX "quash"
#define INTERFACE_SEPARATOR ":"
#define INTERFACE_SUFFIX ">"
#define EXIT 0
#define CONTINUE 1


pid_t fpid = -1;
bool fg_exec = false;
int nextJid = 1;
map<int, int> jid_pid;
extern char ** environ;

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

    // This is hacky. we may have to live with it
    for (int i=0; i<nextJid; i++)
    {
      map<int, int>::iterator jid = jid_pid.find(i);
      if (jid != jid_pid.end() && jid->second == pid) {
        jid_pid.erase(jid);
        break;
      }
    }
  	
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

int 
tokenize(string input, Job *jobs)
{
	string arg;
	int curJob = 0;

	if (input.length() == 0)
		return 0;

	stringstream ss(input);
	jobs[curJob].id = nextJid++;

	while(ss.good())
	{
		ss >> arg;
		
		// arg is empty. do nothing.
		if (arg.find_first_not_of(' ') == string::npos) {;}

		// Increment to the next job if a pipe is passed
		else if (strcmp(arg.c_str(), "|") == 0) {
			curJob++;
			jobs[curJob].id = nextJid;
			nextJid++;

		// Redirect stdout
		} else if (strcmp(arg.c_str(), ">") == 0) {
			ss >> arg;
			jobs[curJob].outputFile = arg;
		
		// Redirect stdin
		} else if (strcmp(arg.c_str(), "<") == 0) {
			ss >> arg;
			jobs[curJob].inputFile = arg;

		// This job will run in the background if we see an ampersand
		} else if (strcmp(arg.c_str(), "&") == 0) {
			jobs[curJob].bg = true;

		// Add the argument to this job's argv list
		} else {
			strcpy(jobs[curJob].argv[jobs[curJob].argc++], arg.c_str());
		}
	}

	return curJob+1;
}


void
redirectStdIn(string &filename)
{
	FILE *f = fopen(filename.c_str(), "r");
	if(f == NULL) 
	{
		perror("file redirect error");
	} else {
		// Rename STDIN.
		dup2(fileno(f), STDIN_FILENO);
		fclose(f);
	}
}

void
redirectStdOut(string &filename)
{
	FILE *f = fopen(filename.c_str(), "w+");
	if(f == NULL)
	{
		perror("file redirect error");
	} else {
		// Rename STDIN.
		dup2(fileno(f), STDOUT_FILENO);
		fclose(f);
	}
}

/*
 * Display command line message to prompt user for input. Return the number 
 * of jobs demanded.
 */
int
prompt(string cwd, Job *jobs)
{
	string input;

	cout << INTERFACE_PREFIX 
		<< INTERFACE_SEPARATOR 
		<< cwd 
		<< INTERFACE_SUFFIX << ' ';

	getline(cin, input);

	return tokenize(input, jobs);
}


/*
 * Execute each given set of commands as a seperate job. if exit is called, 
 * kill all jobs and tell main thread to exit.
 */
int
executeJobs(int numJobs, Job *jobs)
{
	int numPipes = numJobs-1;
	int pipefd[numPipes][2];
	pid_t pid;

	// set up pipe for I/O redirection
	for(int i=0; i<numPipes; i++)
	{
		if (pipe(pipefd[i]) < 0) {
			perror("pipe");
			return EXIT;
		}
	}

	// loop through jobs
	for (int i=0; i<numJobs; i++)
	{

		// Run cd
		if (strcmp(jobs[i].argv[0], "cd") == 0) {
			if (jobs[i].argc < 2)
				cd(getenv("HOME"));
			else if (strcmp(jobs[i].argv[1], "~") == 0)
				cd(getenv("HOME"));
			else if (cd(jobs[i].argv[1]) < 0)
					perror("cd");
		}

		//print jobs
		else if (strcmp(jobs[i].argv[0], "jobs") == 0) {
      for (int j=0; j<nextJid; j++)
      {
        map<int, int>::iterator jid = jid_pid.find(j);

        if (jid != jid_pid.end())
  				printf("[%d] %d %s\n", j, jid->second, jobs[i].argv[0]);
			}
		}

		//print kill
		else if (strcmp(jobs[i].argv[0], "kill") == 0) {
			stringstream ss(jobs[i].argv[1]);
			pid_t value;
			
			ss >> value;

			kill(value, SIGKILL);
		}

		// Run set
		else if (strcmp(jobs[i].argv[0], "set") == 0) {
			
			int overwrite = 1;
			stringstream ss(jobs[i].argv[1]);
			string name, value;
			
			getline(ss, name, ENV_DELIM);
			getline(ss, value, ENV_DELIM);

			if (jobs[i].argc > 1) {
				if (setenv(name.c_str(), value.c_str(), overwrite) < 0)
					perror("set");
			} else {
				cout << "usage: set name=value" << endl;
			}
		}

		// Run get
		else if (strcmp(jobs[i].argv[0], "get") == 0) {
		
			// variable provided
			if (jobs[i].argc > 1) {
				const char *value = getenv(jobs[i].argv[1]);
				
				if (strcmp(value, jobs[i].argv[1]) == 0)
					cout << "Item does not exist" << endl;
				else
					cout << jobs[i].argv[1] 
						<< ":" 
						<< getenv(jobs[i].argv[1]) 
						<< endl;
			
			// no arguments passed
			} else {
				cout << "usage: get environment_variable" << endl;
			}

		}

		// Exit the program
		else if (strcmp(jobs[i].argv[0], "exit") == 0 || 
				 strcmp(jobs[i].argv[0], "quit") == 0) 
		{
			return EXIT;
		}

		// Execute file
		else if(jobs[i].argc > 0) {
			pid = fork();

			// child
			if (pid == 0) {
				
				if (!jobs[i].inputFile.empty() || !jobs[i].outputFile.empty()) {
				
					if (!jobs[i].inputFile.empty()) {
						redirectStdIn(jobs[i].inputFile);
					}

					if (!jobs[i].outputFile.empty()) {
						redirectStdOut(jobs[i].outputFile);
					}

				} else if (numPipes > 0) {
					// this is the first pipe, we only need the write end.
					if (i == 0) {
						if (dup2(pipefd[i][1], STDOUT_FILENO) < 0)
							perror("dup2");

					// middle pipe we need to redirect both ends.
					} else if (i < numPipes) {
						if (dup2(pipefd[i-1][0], STDIN_FILENO) < 0)
							perror("dup2");

						if (dup2(pipefd[i][1], STDOUT_FILENO) < 0)
							perror("dup2");

					// last pipe, close write end and redirect stdin
					} else if (i == numPipes) {
						if (dup2(pipefd[i-1][0], STDIN_FILENO) < 0)
							perror("dup2");
					}
		
					// at this point we aren't using any of the old file
					// descriptors. Go ahead and close them all.
					for (int k=0; k<numPipes; k++)
					{	
						// main already closed this write end.
						// this will fail and set errno.
						if (close(pipefd[k][1]) < 0)
							;
						
						if (close(pipefd[k][0]) < 0)
							perror("close");
					}
				}

				jobs[i].argv[jobs[i].argc] = NULL;

				// set a new process group for background processes
				if (jobs[i].bg)
					setpgid(0, 0);

				// exec file
				// if (execvpe(jobs[i].argv[0], jobs[i].argv, environ) < 0) { //for use with linux
				if (execvp(jobs[i].argv[0], jobs[i].argv) < 0) { //for use with os x
					perror(jobs[i].argv[0]);
					exit(-1);
				}

			// parent
			} else {

        jobs[i].pid = pid;

        // associate this job id with the child's pid
        jid_pid.insert(pair<int, int>(jobs[i].id, jobs[i].pid)); 
				
        // last child is done, close write of last pipe
				if (i > 0) {
					if (close(pipefd[i-1][1]) < 0) {
						perror("close");
						cout << "parent\n";
					}
				}

				// is the current process a background process?
				if (jobs[i].bg) {
					printf("[%d] %d Running in background\n", jobs[i].id, jobs[i].pid);

				// nope, foreground
				} else {
					fpid = pid;
					fg_exec = true;
					while(fg_exec) pause();
				}
			}
		}
	}

	return CONTINUE;
}


/*
 * Quash welcome message
 */
void 
welcome()
{
	ifstream inf(".quashrc");
	cout << "-------------------------------------------------" << endl
		<< "        Quash v1.0.0  Copyright (c) 2014         " << endl
		<< "      Authors: Kendal Harland   Adam Smith       " << endl
		<< "-------------------------------------------------" << endl;

	while (!inf.eof())
	{
		string line;
		getline(inf, line);
		cout << line << endl;
	}	

	cout << "-------------------------------------------------" << endl;	
}
	
/*-----------------------------------------------
   MAIN IMPLEMENTATION
---------------------------------------------- */
int 
main(int argc, char *argv[], char **envp)
{
	char cwd[CWD_BUFSIZE];
	int numJobs, status;

	welcome();
	signal(SIGCHLD, handleChildDone);

	// Loop for user input
	while(true)
	{	
		Job jobs[MAX_JOBS];
		getcwd(cwd, CWD_BUFSIZE);

		numJobs = prompt(cwd, jobs);

		if (numJobs > 0) {
			status = executeJobs(numJobs, jobs);
			if (status == EXIT)
				return 0;
		}
	}

	return 0;
}


