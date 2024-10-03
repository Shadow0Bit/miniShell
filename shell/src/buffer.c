#include <buffer.h>

void
buffer_init(buffer *buf)
{
    buf->begin_ptr = buf->buf;
	buf->line_beg = buf->buf;
	buf->line_end = buf->buf;
	buf->end_ptr = buf->buf + BUF_SIZE;
}