#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>

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

/*
 * Display command line message to prompt user for input
 */
int
prompt(string cwd, string *argv)
{
	string input;
	int argc = 0;

	cout << INTERFACE_PREFIX << INTERFACE_SEPARATOR << cwd << INTERFACE_SUFFIX << ' ';
	getline(cin, input);
	stringstream ss(input);

	while(ss.good())
		ss >> argv[argc++];

	return argc;
}

/*
 * Environment variable initalizations for quash
 */
void 
init(map<string, string> *envVars)
{
	envVars->insert(pair<string, string>("PATH", "./bin"));
	envVars->insert(pair<string, string>("HOME", getenv("HOME")));
	envVars->insert(pair<string, string>("CWD", getenv("HOME")));
}

/* 
 * Deallocated memory and exit program
 */
void
smash(){;}


int 
main(int argc, char *argv[])
{
	map<string, string> envVars;
	string qargv[MAX_ARGS];
	int qargc = 0;

	// Set up the quash environment
	init(&envVars);

	do
	{
		// Prompt user for input
		qargc = prompt(envVars.find("CWD")->second, qargv);
		
	} while(qargv[0].compare("exit") != 0 && qargv[0].compare("quit") != 0);

	//deallocate variables and exit quash
    smash();
	return 0;
}