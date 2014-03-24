#ifndef JOB_H
#define JOB_H
#define MAX_ARGS 100

struct Job
{
	char *qargv[MAX_ARGS];
	int qargc;
	int jid;
};

#endif//JOB_H