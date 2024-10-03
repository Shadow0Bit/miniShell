#include "notes_it.h"

notes_it * 
notes_board_it_init(notes_it *it, notes *_notes)
{
	it->_notes = _notes;
	it->pid = _notes->pid;
	it->status = _notes->status;
	it->index = 0;
	return it;
}

int 
notes_board_it_next(notes_it *it)
{
	if (it->index >= MAX_LINE_LENGTH) {
		it->pid = it->_notes->pid;
		it->status = it->_notes->status;
		it->index = 0;
	} else {
		it->pid++;
		it->status++;
		it->index++;
	}

	return it->index;
}

void 
make_note(notes_it *it, int pid, int status) 
{
	*(it->pid) = pid;
	*(it->status) = status;
	notes_board_it_next(it);
}