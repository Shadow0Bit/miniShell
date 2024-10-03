#ifndef _NOTES_H_
#define _NOTES_H_

#include "config.h"

typedef struct {
	int pid[MAX_LINE_LENGTH];
	int status[MAX_LINE_LENGTH];
} notes;

#endif /* !_NOTES_H_ */