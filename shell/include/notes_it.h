#ifndef _NOTES_IT_H_
#define _NOTES_IT_H_

#include "config.h"
#include "notes.h"

typedef struct {
	notes *_notes;
	int *pid;
	int *status;
	int index;
} notes_it;

notes_it * notes_board_it_init(notes_it *it, notes *_notes);
int notes_board_it_next(notes_it *it);

/*
 * Add note to notes
 * 
 * it - pointer to iterator of notes board
 * pid - pid of process
 * status - status of process
 */
void make_note(notes_it *it, int pid, int status);

#endif /* !_NOTES_IT_H_ */