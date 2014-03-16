#ifndef BUILTIN_H
#define BUILTIN_H

#include <cstdlib>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>

using namespace std;

/* CWD_BUFSIZE
 * buffer size for char buffer the holds the current working directory
 */
#define CWD_BUFSIZE 1024

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
		while (entry = readdir(dir))
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

#endif//BUILTIN_H