#include <sys/wait.h>
#include <signal.h>

#include "proc_container.h"

proc_container fg_proc = { .size = 0 };
proc_container bg_proc = { .size = 0 };

int 
insert_proc(proc_container *proc, int pid) 
{
	if (proc->size >= MAX_LINE_LENGTH/2)
		return -1;

	proc->pid[proc->size] = pid;
	proc->size += 1;
	return 0;
}

int 
is_proc_member(proc_container *proc, int pid) 
{
    if (proc->size == 0)
        return -1;
    
    for (int i = proc->size - 1; i >= 0; i--) {
		if (proc->pid[i] == pid)
			return i;
	}

	return -1;
}

int 
reset_proc_cont(proc_container *proc) 
{
	proc->size = 0;
	return 0;
}

int
remove_proc_id(proc_container *proc, int id) 
{
    if (id >= proc->size)
        return -1;

    if (id != proc->size - 1) 
        proc->pid[id] = proc->pid[proc->size - 1];

    proc->size -= 1;
    return 0;
}