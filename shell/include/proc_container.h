#ifndef _PROC_CONTAINER_H_
#define _PROC_CONTAINER_H_

#include "config.h"

/*
 * Container for processes
 */
typedef struct {
	int pid[MAX_LINE_LENGTH];
	int size;
} proc_container;

extern proc_container fg_proc;
extern proc_container bg_proc;

/*
 * Insert pid to proc_container proc
 * 
 * return 0 if success
 * return -1 if failed
 */
int insert_proc(proc_container *proc, int pid);
int is_proc_member(proc_container *proc, int pid);
int reset_proc_cont(proc_container *proc);
int remove_proc_id(proc_container *proc, int id);

#endif /* !_PROCUTILS_H_ */