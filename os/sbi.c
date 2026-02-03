#include "sbi.h"
#include "types.h"
const uint64 SBI_SET_TIMER = 0;
const uint64 SBI_CONSOLE_PUTCHAR = 1;
const uint64 SBI_CONSOLE_GETCHAR = 2;
const uint64 SBI_CLEAR_IPI = 3;
const uint64 SBI_SEND_IPI = 4;
const uint64 SBI_REMOTE_FENCE_I = 5;
const uint64 SBI_REMOTE_SFENCE_VMA = 6;
const uint64 SBI_REMOTE_SFENCE_VMA_ASID = 7;
const uint64 SBI_SHUTDOWN = 8;

//@param which This is stored in the CPU reg a7 and specfifes which category of service is used, defined above
//@param arg0, arg1, arg2 | These are the actual passed arguments such as the ASCII character 'H'
int inline sbi_call(uint64 which, uint64 arg0, uint64 arg1, uint64 arg2)
{
	//This function takes several arguments which are then placed into CPU registers
	//This is done because the only way that two layers can "share" data is by storing them in registers, which can be pulled later on
	register uint64 a0 asm("a0") = arg0;
	register uint64 a1 asm("a1") = arg1;
	register uint64 a2 asm("a2") = arg2;
	register uint64 a7 asm("a7") = which;
	//When the CPU uses ecall in S-Mode, an exception is called that "traps" control back to M-Mode.
	//The CPU stops running the kernel code and starts running the SBI code
	//This function is the ecall that trigger the change
	asm volatile("ecall"
		     : "=r"(a0)
		     : "r"(a0), "r"(a1), "r"(a2), "r"(a7)
		     : "memory");

	//After the ecall is made in S-mode and M-mode now has control, the SBI looks at the a7 registers and sees that the kernal wants to print a char (for example)
	//The the SBI performs a low-level write to a virtual port that the QEMU provides. Then sends the data to the ternimal process.
	return a0;
}

void console_putchar(int c)
{
	sbi_call(SBI_CONSOLE_PUTCHAR, c, 0, 0);
}

int console_getchar()
{
	return sbi_call(SBI_CONSOLE_GETCHAR, 0, 0, 0);
}

void shutdown()
{
	sbi_call(SBI_SHUTDOWN, 0, 0, 0);
}