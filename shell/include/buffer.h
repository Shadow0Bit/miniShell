#ifndef _BUFFER_H_
#define _BUFFER_H_

#include "config.h"

#define BUF_SIZE (MAX_LINE_LENGTH * 2 + 1)

typedef struct {
	char buf[BUF_SIZE];
	char *begin_ptr;
	char *line_beg;
	char *line_end;
	char *end_ptr;
} buffer;

void buffer_init(buffer *buf);

#endif // !_BUFFER_H_