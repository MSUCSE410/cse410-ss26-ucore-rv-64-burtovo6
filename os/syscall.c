#include "syscall.h"
#include "defs.h"
#include "loader.h"
#include "syscall_ids.h"
#include "timer.h"
#include "trap.h"

#define MAX_SYSCALL_NUM 500

uint64 sys_write(int fd, uint64 va, uint len)
{
	debugf("sys_write fd = %d va = %x, len = %d", fd, va, len);
	if (fd != STDOUT)
		return -1;
	struct proc *p = curr_proc();
	char str[MAX_STR_LEN];
	int size = copyinstr(p->pagetable, str, va, MIN(len, MAX_STR_LEN));
	debugf("size = %d", size);
	for (int i = 0; i < size; ++i) {
		console_putchar(str[i]);
	}
	return size;
}

__attribute__((noreturn)) void sys_exit(int code)
{
	exit(code);
	__builtin_unreachable();
}

uint64 sys_sched_yield()
{
	yield();
	return 0;
}

uint64 sys_gettimeofday(TimeVal *val, int _tz) // TODO: implement sys_gettimeofday in pagetable. (VA to PA)
{
	// YOUR CODE
	// Get the currrent processes page table
	pagetable_t proc_pg = curr_proc()->pagetable;

	// Translate the virtual address to a physical address
	uint64 pa = useraddr(proc_pg, (uint64)val);

	TimeVal *ptr = (TimeVal *)pa;
	uint64 cycle = get_cycle();
	ptr->sec = cycle / CPU_FREQ;
	ptr->usec = (cycle % CPU_FREQ) * 1000000 / CPU_FREQ;

	return 0;
}

// TODO: add support for mmap and munmap syscall.
// hint: read through docstrings in vm.c. Watching CH4 video may also help.
// Note the return value and PTE flags (especially U,X,W,R)
uint64 sys_mmap(uint64 start, uint64 len, int port, int flag, int fd)
{
	//Check if the address is page aligned
	if (PGALIGNED(start))
		return -1;
	
	// Round up len to nearest page
	// This function may only be for pages but the idea stands
	len = PGROUNDUP(len);

	//Check if the virtual memory range[start, start+len] is occupied by something else the process is using
	// Might just need to walk through from start to start + len and check if everything is 0

	//Calculate how many pages are needed
	// Need to find where this could be, they will ask where I found this. At least the page_size - 1
	int numOfPages = (len + PAGE_SIZE - 1) / PAGE_SIZE;

	//Allocate the physical pages
	void *pages [numOfPages];
	for (int i = 0; i < numOfPages; i++)
	{
		pages[i] = kalloc();
	}

	//Call mappages() with all the new physical pages and the virtual addresses from start to start+len
	pagetable_t proc_pg = curr_proc()->pagetable;
	for (int i = 0; i < numOfPages; i++)
	{
		mappages(proc_pg, start + (i*PAGE_SIZE), (uint64)pages[i], PAGE_SIZE, 1);
	}
	return 0;
}



int sys_task_info(TaskInfo *ti)
{
	// Get the currrent processes page table
	pagetable_t proc_pg = curr_proc()->pagetable;

	// Translate the virtual address to a physical address
	uint64 pa = useraddr(proc_pg, (uint64)ti);

	TaskInfo *proc_ti = (TaskInfo *)pa;
	proc_ti->status = translate_state(curr_proc()->state);
	for (int i = 0; i < MAX_SYSCALL_NUM; i++){
		proc_ti->syscall_times[i] = curr_proc()->syscall_times[i];
	}

	//uint64 ms_curr_time = (get_cycle() % CPU_FREQ) * 1000 / CPU_FREQ;
	uint64 ms_start_time = (curr_proc()->start_time % CPU_FREQ) * 1000 / CPU_FREQ;
	proc_ti->time = ms_start_time;
	//- ms_start_time;
	
	return 0;
}

extern char trap_page[];

void syscall()
{
	struct trapframe *trapframe = curr_proc()->trapframe;
	int id = trapframe->a7, ret;
	uint64 args[6] = { trapframe->a0, trapframe->a1, trapframe->a2,
			   trapframe->a3, trapframe->a4, trapframe->a5 };
	tracef("syscall %d args = [%x, %x, %x, %x, %x, %x]", id, args[0],
	       args[1], args[2], args[3], args[4], args[5]);

	switch (id) {
	case SYS_write:
		curr_proc()->syscall_times[SYS_write] += 1;
		ret = sys_write(args[0], args[1], args[2]);
		break;
	case SYS_exit:
		sys_exit(args[0]);
		// __builtin_unreachable();
	case SYS_sched_yield:
		curr_proc()->syscall_times[SYS_sched_yield] += 1;
		ret = sys_sched_yield();
		break;
	case SYS_gettimeofday:
		curr_proc()->syscall_times[SYS_gettimeofday] += 1;
		ret = sys_gettimeofday((TimeVal *)args[0], args[1]);
		break;
	case SYS_task_info:
		curr_proc()->syscall_times[SYS_task_info] += 1;
		ret = sys_task_info((TaskInfo *)args[0]);
		break;
	default:
		ret = -1;
		errorf("unknown syscall %d", id);
	}
	trapframe->a0 = ret;
	tracef("syscall ret %d", ret);
}
