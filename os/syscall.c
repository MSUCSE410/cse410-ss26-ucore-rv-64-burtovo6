#include "syscall.h"
#include "defs.h"
#include "loader.h"
#include "syscall_ids.h"
#include "timer.h"
#include "trap.h"

uint64 sys_write(int fd, char *str, uint len)
{
	debugf("sys_write fd = %d str = %x, len = %d", fd, str, len);
	if (fd != STDOUT)
		return -1;
	for (int i = 0; i < len; ++i) {
		console_putchar(str[i]);
	}
	return len;
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

uint64 sys_gettimeofday(TimeVal *val, int _tz)
{
	uint64 cycle = get_cycle();
	val->sec = cycle / CPU_FREQ;
	val->usec = (cycle % CPU_FREQ) * 1000000 / CPU_FREQ;
	return 0;
}

/*
* LAB1: you may need to define sys_task_info here
* Here is where we are defining the happenings of the sys_call (I think at this point we are assigning the values to *ti)
*/
int sys_task_info(TaskInfo *ti)
{
	TaskInfo *proc_ti = (TaskInfo *)ti;
	proc_ti->status = translate_state(curr_proc()->state);
	for (int i = 0; i < 500; i++){
		proc_ti->syscall_times[i] = curr_proc()->syscall_times[i];
	}

	uint64 ms_curr_time = (get_cycle() % CPU_FREQ) * 1000 / CPU_FREQ;
	uint64 ms_start_time = (curr_proc()->start_time % CPU_FREQ) * 1000 / CPU_FREQ;
	proc_ti->time = ms_curr_time - ms_start_time;
	
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
	/*
	* LAB1: you may need to update syscall counter for task info here
	*/
	switch (id) {
	case SYS_write:
		curr_proc()->syscall_times[SYS_write] += 1;
		ret = sys_write(args[0], (char *)args[1], args[2]);
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
	/*
	* LAB1: you may need to add SYS_taskinfo case here
	* Here is where we are delatgating the system call to the correct function
	*/
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
