#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>

#include "builtins.h"

bool convertstr(char * to_convert, long * num);
int echo(char*[]);
int undefined(char *[]);
int change_directory(char * argv[]);
int exit_shell(char * argv[]);
int kill_builtin(char * argv[]);
int lls(char * argv[]);
void print_builtin_error();
int wlr(char * argv[]);

builtin_pair builtins_table[]={
	{"exit",	&exit_shell},
	{"lecho",	&echo},
	{"lcd",		&change_directory},
	{"cd",		&change_directory},
	{"lkill",	&kill_builtin},
	{"lls",		&lls},
	{"wlr",		&wlr},
	{NULL,NULL}
};

bool
convertstr(char * to_convert, long * num) {
	char * end;
	long _num = strtol(to_convert, &end, 10);

	if (_num > INT_MAX || _num < INT_MIN)
		return false;

	if (*end != '\0' || to_convert == end)
		return false;
	
	*num = _num;
	return true;
}

int 
echo(char * argv[])
{
	int i = 1;
	if (argv[i]) printf("%s", argv[i++]);
	while  (argv[i])
		printf(" %s", argv[i++]);

	printf("\n");
	return 0;
}

int 
undefined(char * argv[])
{
	fprintf(stderr, "Command %s undefined.\n", argv[0]);
	return BUILTIN_ERROR;
}

int
change_directory(char * argv[]) {
	int chdir_ret;
	if (argv[1] == 0) 
		chdir_ret = chdir(getenv("HOME"));
	else if (argv[2] == 0) 
		chdir_ret = chdir(argv[1]);
	
	if (argv[2] != 0 || chdir_ret == -1) {
		fprintf(stderr, "Builtin lcd error.\n");
		return BUILTIN_ERROR;
	}

	return 0;
}

int
exit_shell(char * argv[]) {
	exit(0);
}

int
kill_builtin(char * argv[]) {
	if (argv[1] == 0 || (argv[2] == 0 && argv[1][0] == '-')) {
		fprintf(stderr, "Builtin lkill error.\n");
		return BUILTIN_ERROR;
	}

	long pid, signal_num;
	if (argv[2] == 0 && argv[1][0] != '-') {
		signal_num = SIGTERM;
		if(!convertstr(&argv[1][0], &pid)) {
			fprintf(stderr, "Builtin lkill error.\n");
			return BUILTIN_ERROR;
		}
	} else {
		if(!convertstr(&argv[1][1], &signal_num) || !convertstr(&argv[2][0], &pid)) {
			fprintf(stderr, "Builtin lkill error.\n");
			return BUILTIN_ERROR;
		}
	}

	kill(pid, signal_num);
	return 0;
}

int
lls(char * argv[]) {
	if (argv[1] != 0) {
		fprintf(stderr, "Builtin lls error.\n");
		return BUILTIN_ERROR;
	}

	DIR* dir = opendir(".");
	if (dir == NULL) {
		fprintf(stderr, "Builtin lls error.\n");
		return BUILTIN_ERROR;
	}

	struct dirent *de = readdir(dir);
	while (de != NULL) {
		if (de->d_name[0] != '.') {
			printf("%s\n", de->d_name);
		}
		de = readdir(dir);
	}

	closedir(dir);

	fflush(stdout);
	return 0;
}

int 
wlr(char * argv[]) {
	printf("🩸🧛🩸\n");
	return 0;
}