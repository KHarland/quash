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
 * buffer size for char buffer the holds the current working directory name
 */
#define CWD_BUFSIZE 1024

/* 
 * Change working directory to path
 */
int
cd(const char* path)
{
	if (chdir(path) < 0)
		return -1;
	return 1;
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
 * Tell if the given filename is hidden
 */
bool isHidden(const char *fname)
{
	return fname[0] == '.'; 
}

/*
 * List the specified path
 */
void
ls(const char *path, char *flags, int numFlags) 
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
			} else if (!isHidden(entry->d_name)){
				cout << entry->d_name << endl;
			}
		}
		closedir(dir);
	}
}

#endif//BUILTIN_H
