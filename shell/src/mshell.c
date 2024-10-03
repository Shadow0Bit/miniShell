#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>

#include "buffer.h"
#include "config.h"
#include "siparse.h"
#include "utils.h"
#include "builtins.h"
#include "proc_container.h"
#include "notes.h"
#include "notes_it.h"
#include "sighandling.h"

notes bg_notes;
notes_it bg_notes_it;
notes_it bg_notes_start;
sigset_t child_sigset;
sigset_t empty_sigset;
struct sigaction sa_chld;

int comseq_run(commandseq *comseq, int flags);
int redirect(redirseq *redirseq);
int exec_comand(char *args[]);
int builtin_search(command *com, char *args[]);
void print_prompt(bool is_tty);
void print_notes();
void print_syntax_error();
void print_errno(char *file_name, char *default_error_ms);

int 
main(int argc, char *argv[]) 
{	
	command *com;
	pipelineseq *pipeseq;
	buffer buf;
	ssize_t nread;

	buffer_init(&buf);
	notes_board_it_init(&bg_notes_it, &bg_notes);
	notes_board_it_init(&bg_notes_start, &bg_notes);
	handler_init(&sa_chld, &bg_notes_it);
	
	sigemptyset(&empty_sigset); 
    sigaddset(&child_sigset, SIGCHLD);
	signal(SIGINT, SIG_IGN);

	bool is_terminal = false;
	if (isatty(STDIN_FILENO))
		is_terminal = true;
	
	print_prompt(is_terminal);

	while (nread = read(STDIN_FILENO, buf.begin_ptr, buf.end_ptr - buf.begin_ptr)) {
		if (nread == -1) {
			continue;
		}

		while (buf.line_end = memchr(buf.line_beg, '\n', (buf.begin_ptr + nread) - buf.line_beg)) {
			*buf.line_end = '\0';

			if (buf.line_end - buf.line_beg > MAX_LINE_LENGTH) {
				print_syntax_error();
				*buf.line_end = 0;
				buf.line_beg = buf.line_end + 1;
				continue;
			}

			pipeseq = parseline(buf.line_beg);
			pipeseq->prev->next = NULL;
			while (pipeseq != NULL) {
				comseq_run(pipeseq->pipeline->commands, pipeseq->pipeline->flags);
				pipeseq = pipeseq->next;
			}
			
			buf.line_beg = buf.line_end + 1;	
		}

		if (buf.line_end == NULL) {
			if ((buf.begin_ptr + nread) - buf.line_beg >= MAX_LINE_LENGTH)
				buf.begin_ptr = buf.line_beg + MAX_LINE_LENGTH;
			else
				buf.begin_ptr = buf.begin_ptr + nread;
			
			if ((buf.end_ptr - buf.begin_ptr) <= MAX_LINE_LENGTH) {
				buf.begin_ptr = buf.buf + (buf.begin_ptr - buf.line_beg);
				memmove(buf.buf, buf.line_beg, buf.begin_ptr - buf.buf);
				buf.line_beg = buf.buf;		
			}
		}

		if (nread == 0)
			break;

		print_prompt(is_terminal);
	}

	return 0;
}

/*
 *comseq_run iterates throught commands,
 *forks if necessary (if com is not a builtin)
 *and then executes the command
 *returns 0 on success, -1 on failure
 */
int
comseq_run(commandseq *comseq, int flags)
{
	/* pipefd[0] - in, pipefd[1] - out */
	int pipefd[2] = {-1, -1};
	int old_pipefd[2] = {-1, -1};

	block_sigset(&child_sigset);
	comseq->prev->next = NULL;
	while (comseq != NULL) {
		command *com = comseq->com;

		if (com == NULL) {
			break;
		}

		char *args[MAX_LINE_LENGTH/2];
		/* Copy command arguments to the args array */
		argseq *argseq = com->args;
		for (int i = 0; i < MAX_LINE_LENGTH/2; i++) {
			args[i] = argseq->arg;
			argseq = argseq->next;
			if (argseq == com->args) {
				args[i + 1] = 0;
				break;
			}
		}

		int builtin_ret = 0;
		if (com->redirs == NULL) 
			builtin_ret = builtin_search(com, args);

		if (builtin_ret == 1)
			return 0;
		else if (builtin_ret == -1)
			return -1;

		if (comseq->next != NULL) {
			if (pipe(pipefd) == -1) {
				printf("Error while creating pipe\n");
				unblock_sigset(&child_sigset);
				return -1;
			}
		}

		int child_pid = fork();
		if (child_pid == -1) {
			printf("Error while forking\n");
			return -1;
		} else if (child_pid == 0) {
			/* set process group */
			signal(SIGINT, SIG_DFL);

			if (flags == INBACKGROUND) 
				setsid();

			/* child behavior */
			if (old_pipefd[0] != -1) {
				dup2(old_pipefd[0], STDIN_FILENO);
				close(old_pipefd[0]);
				close(old_pipefd[1]);
			}

			if (comseq->next != NULL) { 
				dup2(pipefd[1], STDOUT_FILENO);
				close(pipefd[0]);
				close(pipefd[1]);
			}

			redirect(com->redirs);
			unblock_sigset(&child_sigset);
			exec_comand(args);
		} else {
			/* parent behavior */
			if (old_pipefd[0] != -1) {
				close(old_pipefd[0]);
				close(old_pipefd[1]);
			}
			old_pipefd[0] = pipefd[0];
			old_pipefd[1] = pipefd[1];

			if (flags == INBACKGROUND)
				insert_proc(&bg_proc, child_pid);
			else
				insert_proc(&fg_proc, child_pid);

			comseq = comseq->next;
		}
	}
	unblock_sigset(&child_sigset);

	if (pipefd[0] != -1) {
		close(pipefd[0]);
		close(pipefd[1]);
	}

	/* wait for all children to finish */
	block_sigset(&child_sigset);
	while (fg_proc.size > 0) {
		sigsuspend(&empty_sigset);
	}
	unblock_sigset(&child_sigset);

	return 0;
}

/*
 * redir redirects input/output acording to
 * the redirections from the redirseq list
 */
int 
redirect(redirseq *redirseq) 
{
	if (redirseq == NULL)
		return 0;

	char *default_msg = "exec error";
	redirseq->prev->next = NULL;
	while (redirseq != NULL) {
		int fd;
		if (IS_RIN(redirseq->r->flags)) {
			fd = open(redirseq->r->filename, O_RDONLY);
			if (fd == -1) {
				print_errno(redirseq->r->filename, default_msg);
				exit(EXEC_FAILURE);
			}
			dup2(fd, STDIN_FILENO);
		} else if (IS_RAPPEND(redirseq->r->flags)) {
			fd = open(redirseq->r->filename, O_WRONLY | O_APPEND | O_CREAT, 0644);
			if (fd == -1) {
				print_errno(redirseq->r->filename, default_msg);
				exit(EXEC_FAILURE);
			}
			dup2(fd, STDOUT_FILENO);
		} else if (IS_ROUT(redirseq->r->flags)){
			fd = open(redirseq->r->filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
			if (fd == -1) {
				print_errno(redirseq->r->filename, default_msg);
				exit(EXEC_FAILURE);
			}
			dup2(fd, STDOUT_FILENO);
		}
		close(fd);
		redirseq = redirseq->next;
	}

	return 0;
}

/*
 * exec_comand executes the command com
 * using execvp function. It handles execvp
 * faliure.
 */
int 
exec_comand(char *args[]) {
	int exec = execvp(args[0], args);
		/* handle execvp faliure */
		if (exec == -1) {
			print_errno(args[0], "exec error");
			exit(EXEC_FAILURE);
		}
}

/*
 * builtin_search searches for a builtin command
 * in the builtins_table and executes it if found
 * returns 1 on success, -1 on failure and
 * 0 if command is not a builtin
 */
int
builtin_search(command *com, char *args[]) {
	builtin_pair *builtin;
	int i = 0;
	while (builtin = &builtins_table[i++]) {
		if (builtin->name == NULL)
			break;
		
		if (strcmp(builtin->name, args[0]) == 0) {
			int builtin_ret = builtin->fun(args);
			if (builtin_ret == BUILTIN_ERROR)
				return -1;
			else
				return 1;
		}
	}

	return 0;
}

void
print_notes() 
{
	sigprocmask(SIG_BLOCK, &child_sigset, NULL);
	while (bg_notes_start.index != bg_notes_it.index) {
		char *status_str;
		if (WIFSIGNALED(*bg_notes_start.status)) {
			status_str = "(killed by signal";
			*bg_notes_start.status = WTERMSIG(*bg_notes_start.status);
		} else if (WIFEXITED(*bg_notes_start.status)) {
			status_str = "(exited with status";
			*bg_notes_start.status = WEXITSTATUS(*bg_notes_start.status);
		} else {
			status_str = "unknown";
		}

		printf("Background process %d terminated. %s %d)\n", *bg_notes_start.pid, status_str, *bg_notes_start.status);
		notes_board_it_next(&bg_notes_start);
	}
	sigprocmask(SIG_UNBLOCK, &child_sigset, NULL);
}

/*
 * print_prompt prints notes and the prompt string
 * if is_tty is true.
 */
void 
print_prompt(bool is_tty)
{
	if (is_tty) {
		print_notes();
		printf("%s", PROMPT_STR);
		fflush(stdout);
	}
}

void 
print_syntax_error() 
{
	fprintf(stderr, "%s\n", SYNTAX_ERROR_STR);
	fflush(stderr);
}

/*
 * print_errno prints error messages based on 
 * the value of the global errno variable and 
 * the provided program_name. It handles three cases.
 */
void 
print_errno(char *file_name, char *default_error_msg) 
{
	switch (errno) {
		case EACCES:
			fprintf(stderr, "%s: permission denied\n", file_name);
			break;

		case ENOENT:
			fprintf(stderr, "%s: no such file or directory\n", file_name);
			break;
	
		default:
			fprintf(stderr, "%s\n", default_error_msg);
			break;
	}

	fflush(stderr);
}