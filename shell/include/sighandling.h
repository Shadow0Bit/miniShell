#ifndef _SIGHANDLING_H_
#define _SIGHANDLING_H_

#include <signal.h>

#include "notes_it.h"

/*
 * SIGCHLD handler
 * 
 * Removes process from fg_proc or bg_proc
 * and adds note to notes board
 */
void sigchld_handler(int sig);

/*
 * Initialize handler and sigaction struct
 * 
 * sa - pointer to sigaction struct
 * bg_notes_it - pointer to iterator of notes board
 */
int handler_init(struct sigaction *sa, notes_it *bg_notes_it);

/*
 * Block sigset
 * if block failed, 
 * print error message and exit
 */
void block_sigset(sigset_t *sigset);

/*
 * Unblock sigset 
 * if block failed, 
 * print error message and exit
 */
void unblock_sigset(sigset_t *sigset);

#endif /* !_SIGHANDLING_H_ */