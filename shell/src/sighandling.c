#include <sys/wait.h>
#include <stdio.h>

#include "sighandling.h"
#include "proc_container.h"

notes_it *it;

int
handler_init(struct sigaction *sa, notes_it *bg_notes_it)
{
    it = bg_notes_it;
    sa->sa_handler = sigchld_handler;
	sa->sa_flags = SA_NOCLDSTOP;
	sigemptyset(&sa->sa_mask);
	sigaction(SIGCHLD, sa, NULL);
}

void
block_sigset(sigset_t *sigset)
{
    int success = sigprocmask(SIG_BLOCK, &sigset, NULL); //!TODO: it's ugly, refactor that, maybe add sme kind of wrapper
	if (success == -1) {
		printf("Error while blocking SIGCHLD\n");
		exit(-1);
	}
}

void
unblock_sigset(sigset_t *sigset)
{
    int success = sigprocmask(SIG_UNBLOCK, &sigset, NULL);
    if (success == -1) {
        printf("Error while unblocking SIGCHLD\n");
        exit(-1);
    }
}

void 
sigchld_handler(int sig) 
{
    int pid;
	int status;
	int fg_id, bg_id;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		if ((bg_id = is_proc_member(&bg_proc, pid)) != -1) {
            make_note(it, pid, status);
			remove_proc_id(&bg_proc, bg_id);
		} else if ((fg_id = is_proc_member(&fg_proc, pid)) != -1) {
            remove_proc_id(&fg_proc, fg_id);
		}
    }
}