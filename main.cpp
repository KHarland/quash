//C++ libs
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <typeinfo>
//Unix libs
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>

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

/* INTERFACE_[PREFX|SEPARATOR|SUFFIX]
 * These constants make up the title that gets displayed when the user is 
 * prompted for input via the command line. 
 */
#define INTERFACE_PREFIX "quash"
#define INTERFACE_SEPARATOR ":"
#define INTERFACE_SUFFIX ">>"

using namespace std;


/*-----------------------------------------------
   BUILT-IN SYSTEM TOOLS
---------------------------------------------- */

/* 
 * Change working directory to path
 */
void
cd(const char* path)
{
	if (chdir(path) < 0)
		perror("error");
}

/* 
 * Print the current working directory
 */
void
pwd()
{
	char cwd[CWD_BUFSIZE];

	if(getcwd(cwd, CWD_BUFSIZE) == NULL)
		perror("error");
	else
		cout << cwd << endl;
}

/*
 * List the specified path
 */
void
ls(const char* path)
{
	int lasterr = errno;
	DIR *dir;
	const dirent *entry;
	
    dir = opendir(path);

	if (dir == NULL) {
		perror("error");
	} else {
		while ((entry = readdir(dir)))
		{
			if (entry == NULL) {
				if (lasterr != errno)
					perror("error");
				else 
					break;
			} else {
				cout << entry->d_name << endl;
			}
		}
		
		closedir(dir);
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
 * Display command line message to prompt user for input
 */
int
prompt(string cwd, char *qargv[])
{
	string input, qarg;
	int qargc = 0;

	cout << INTERFACE_PREFIX << INTERFACE_SEPARATOR << cwd << INTERFACE_SUFFIX << ' ';
	getline(cin, input);
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
	envVars->insert(pair<string, string>("PATH", "./bin"));
	envVars->insert(pair<string, string>("HOME", getenv("HOME")));
	chdir(getenv("HOME"));
}

/* 
 * Deallocated memory and exit program
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
	int qargc = 0;
	char cwd[CWD_BUFSIZE];

	// Set up the quash environment
	init(&envVars);

	// Set up space for input args
	for(int i=0; i<MAX_ARGS; i++)
		qargv[i] = new char[256];

	// User input loop
	do
	{
		// Get command
		getcwd(cwd, CWD_BUFSIZE);
		qargc = prompt(cwd, qargv);

		// Run cd
		if (strcmp(qargv[0], "cd") == 0) {
			if (qargc < 2)
				cd(getenv("HOME"));
			else
				cd(qargv[1]);
		}

		// Run pwd
		else if (strcmp(qargv[0], "pwd") == 0) {
			pwd();
		}

		// Run ls
		else if(strcmp(qargv[0], "ls") == 0) {
			if (qargc < 2)
				ls(cwd);
			else
				ls(qargv[1]);
		}

	} while(strcmp(qargv[0], "exit") != 0 && strcmp(qargv[0], "quit") != 0);

	//deallocate variables and exit quash
	for(int i=0; i<MAX_ARGS; i++)
		delete [] qargv[i];

	smash();

	return 0;
}
