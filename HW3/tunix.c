/* file: tunix.c core kernel code */

#include <cpu.h>
#include <gates.h>
#include "tsyscall.h"
#include "tsystm.h"
#include "proc.h"
#include "sched.h"

extern IntHandler syscall; /* the assembler envelope routine    */
//extern void ustart(void);
extern void finale(void);

/* kprintf is proto'd in stdio.h, but we don't need that for anything else */
void kprintf(char *, ...);

/* functions in this file */
void debug_set_trap_gate(int n, IntHandler *inthand_addr, int debug);
void set_trap_gate(int n, IntHandler *inthand_addr);
int sysexit(int);
void k_init(void);
void shutdown(void);
void syscallc(int user_eax, int devcode, char *buff, int bufflen);

/* Record debug info in otherwise free memory between program and stack */
/* 0x300000 = 3M, the start of the last M of user memory on the SAPC */
#define DEBUG_AREA 0x300000
char *debug_log_area = (char *) DEBUG_AREA;
char *debug_record; /* current pointer into log area */

#define MAX_CALL 6

/* optional part: syscall dispatch table */
static struct sysent {
	short sy_narg; /* total number of arguments */
	int (*sy_call)(int, ...); /* handler */
} sysent[MAX_CALL];

//Functions used in homework 3
extern void ustart1(void), ustart2(void), ustart3(void);
void init_proctab(void);
void process0(void);

PEntry proctab[NPROC], *curproc;
int zombie_count_gbl;

#define ESP1 0x280000	//process 1 stack addr
#define ESP2 0x290000	//process 2 stack addr 
#define ESP3 0x2a0000	//process 3 stack addr 

/****************************************************************************/
/* k_init: this function for the initialize  of the kernel system*/
/****************************************************************************/

void k_init() {
	debug_record = debug_log_area; /* clear debug log */
	cli();
	ioinit(); /* initialize the deivce */
	set_trap_gate(0x80, &syscall); /* SET THE TRAP GATE*/

	/* Note: Could set these with array initializers */
	/* Need to cast function pointer type to keep ANSI C happy */
	sysent[TREAD].sy_call = (int (*)(int, ...)) sysread;
	sysent[TWRITE].sy_call = (int (*)(int, ...)) syswrite;
	sysent[TEXIT].sy_call = (int (*)(int, ...)) sysexit;

	sysent[TEXIT].sy_narg = 1; /* set the arg number of function */
	sysent[TREAD].sy_narg = 3;
	sysent[TIOCTL].sy_narg = 3;
	sysent[TWRITE].sy_narg = 3;
	sti(); /* user runs with interrupts on */
	init_proctab(); //Initial program
	process0();     //schedule program till everything is a zombie task and shutdown
}

/* shut the system down */
void shutdown() {
	kprintf("\nSHUTTING THE SYSTEM DOWN!\n");
	kprintf("Debug log from run:\n");
	kprintf("Marking kernel events as follows:\n");
	kprintf("  ^a   COM2 input interrupt, a received\n");
	kprintf("  ~    COM2 output interrupt, ordinary char output\n");
	kprintf("  ~e   COM2 output interrupt, echo output\n");
	kprintf("  ~s   COM2 output interrupt, shutdown TX ints\n");
	kprintf("%s", debug_log_area); /* the debug log from memory */
	kprintf("\nLEAVE KERNEL!\n\n");
	finale(); /* trap to Tutor */
}

/****************************************************************************/
/* syscallc: this function for the C part of the 0x80 trap handler          */
/* OK to just switch on the system call number here                         */
/* By putting the return value of syswrite, etc. in user_eax, it gets       */
/* popped back in sysentry.s and returned to user in eax                    */
/****************************************************************************/

void syscallc(int user_eax, int devcode, char *buff, int bufflen) {
	int nargs;
	int syscall_no = user_eax;

	switch (nargs = sysent[syscall_no].sy_narg) {
	case 1: /* 1-argument system call */
		user_eax = sysent[syscall_no].sy_call(devcode); /* sysexit */
		break;
	case 3: /* 3-arg system call: calls sysread or syswrite */
		user_eax = sysent[syscall_no].sy_call(devcode, buff, bufflen);
		break;
	default:
		kprintf("bad # syscall args %d, syscall #%d\n", nargs, syscall_no);
	}
}

/****************************************************************************/
/* sysexit: this function for the exit syscall fuction */
/****************************************************************************/

int sysexit(int exit_code) {
    cli();
    curproc->p_exitval = exit_code;
    curproc->p_status = ZOMBIE;
    zombie_count_gbl++;
    schedule();
    /* never returns */
    return 0;    /* never happens, but avoids warning about no return value */
}

/****************************************************************************/
/* set_trap_gate: this function for setting the trap gate */
/****************************************************************************/
extern void locate_idt(unsigned int *limitp, char ** idtp);

void set_trap_gate(int n, IntHandler *inthand_addr) {
	debug_set_trap_gate(n, inthand_addr, 0);
}

/* write the nth idt descriptor as a trap gate to inthand_addr */
void debug_set_trap_gate(int n, IntHandler *inthand_addr, int debug) {
	char *idt_addr;
	Gate_descriptor *idt, *desc;
	unsigned int limit = 0;

	if (debug)
		kprintf("Calling locate_idt to do sidt instruction...\n");
	locate_idt(&limit, &idt_addr);
	/* convert to CS seg offset, i.e., ordinary address, then to typed pointer */
	idt = (Gate_descriptor *) (idt_addr - KERNEL_BASE_LA);
	if (debug)
		kprintf("Found idt at %x, lim %x\n", idt, limit);
	desc = &idt[n]; /* select nth descriptor in idt table */
	/* fill in descriptor */
	if (debug)
		kprintf("Filling in desc at %x with addr %x\n", (unsigned int) desc,
				(unsigned int) inthand_addr);
	desc->selector = KERNEL_CS; /* CS seg selector for int. handler */
	desc->addr_hi = ((unsigned int) inthand_addr) >> 16; /* CS seg offset of inthand */
	desc->addr_lo = ((unsigned int) inthand_addr) & 0xffff;
	desc->flags = GATE_P | GATE_DPL_KERNEL | GATE_TRAPGATE; /* valid, trap */
	desc->zero = 0;
}

/* append msg to memory log */
void debug_log(char *msg) {
	strcpy(debug_record, msg);
	debug_record += strlen(msg);
}

//Starts the 3 processes, and stores them in the stack space allocated ealier
//
void init_proctab() {
	int i;

        //Store the 3 programs in memory
	proctab[1].p_savedregs[SAVED_PC] = (int) &ustart1;
	proctab[2].p_savedregs[SAVED_PC] = (int) &ustart2;
	proctab[3].p_savedregs[SAVED_PC] = (int) &ustart3;

        //place 3 processes in stack space
	proctab[1].p_savedregs[SAVED_ESP] = ESP1;
	proctab[2].p_savedregs[SAVED_ESP] = ESP2;
	proctab[3].p_savedregs[SAVED_ESP] = ESP3;

        //Loop through processes and set ebp,eflags and set to run
	for (i = 0; i < NPROC; i++) {
		proctab[i].p_savedregs[SAVED_EBP] = 0;
		proctab[i].p_savedregs[SAVED_EFLAGS] = (1 << 9); 
		proctab[i].p_status = RUN;
	}

	curproc = &proctab[0]; // Need to start from Kernel

	zombie_count_gbl = 0;
}

//Continue scheduling until every process is a zombie process
void process0() {

	int i;

	while ((proctab[1].p_status != ZOMBIE) || (proctab[2].p_status != ZOMBIE) || (proctab[3].p_status != ZOMBIE)) {
	schedule();
	}
        
        //wait for buffer to finish
        for (i=0; i< 1000000; i++)
	;		
        
        //print debuf log and finish program
	shutdown();

}



