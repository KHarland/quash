#ifndef JOB_H
#define JOB_H
#define MAX_ARGS 100

struct Job
{
	char *argv[MAX_ARGS];
	int argc;
	int id;
	int pid;
	bool bg;
	bool alive;
	string inputFile;
	string outputFile;

	Job()
	{
		for(int i=0; i<MAX_ARGS; i++)
			argv[i] = new char[256];

		argc = 0;
		id = 0;
		bg = false;
		alive = false;
		inputFile = "";
		outputFile = "";
	}

	~Job()
	{
		//deallocate variables and exit quash
		for(int i=0; i<MAX_ARGS; i++)
			delete [] argv[i];
	}
};

#endif//JOB_H