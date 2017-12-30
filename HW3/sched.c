// Sched.c

#include <stdio.h>
#include "proc.h"
#include "sched.h"
#define BUFLEN 200

extern void debug_log(char *msg);

//Schedule tasks for this iteration. This is called until each
//task is set to a zombie task
void schedule() {

	PEntry *oldproc = curproc;
	int i;
	int found_one_to_run = 0;
	char buf[BUFLEN] = "";

	cli(); //diable ints

        //Loop through processes
	for (i = 1; i <= 3; i++) {
		if (proctab[i].p_status == RUN) { // start process with status RUN
			found_one_to_run = 1;
			curproc = &proctab[i];
			break;
		}
	}

        //If all processes are currently blocked we use 0
	if (found_one_to_run == 0) {
		curproc = &proctab[0]; 
	}

        //Helper function to print to debug log
	if (oldproc != curproc)
		print_process_switch(oldproc);

        //if debug log is not empty, we appent to debug log
	if (strlen(buf))
		debug_log(buf);

        //switch process
	asmswtch(oldproc, curproc);
    sti(); //enable ints
}

//Table to switch processes and print to debug log
static void print_process_switch(PEntry *oldproc) {
	char buf[BUFLEN] = "";

        //Checks the status, and depending on the status it will print the corresponding
        //tag value. z for zombie, b for blocked, and values for misc.
	if (oldproc->p_status == ZOMBIE) {
		sprintf(buf, "|(%dz-%d)", oldproc - proctab, curproc - proctab);
	} else if (oldproc->p_status == BLOCKED) {
		sprintf(buf, "|(%db-%d)", oldproc - proctab, curproc - proctab);
	} else {
		sprintf(buf, "|(%d-%d)", oldproc - proctab, curproc - proctab);
	}
        //stores in debug log if not empty
	if (strlen(buf))
		debug_log(buf);
}

//Takes an event, and sets the process to sleep. 
//Prints that to the debuglog
void sleep(WaitCode event)
{

  char buf[BUFLEN] = "";
  cli(); //disable ints
  sprintf(buf, "sleep:%d", curproc - proctab);

  curproc->p_status = BLOCKED;
  curproc->p_waitcode = event;

  //If the size of the buffer is greater than zero, then 
  //we will add to debug log.
  if (strlen(buf))
	debug_log(buf);

   schedule();
   sti(); //enable ints

}

//This function takes a event code, and if that
// event is found and if the process is blocked 
//then it starts it up again.
void wakeup(WaitCode event)
{
    int i;
    cli(); //diable ints

    //Loops though NPOC till we find the event to run.
    for(i = 1; i < NPROC; i++)
	if((proctab[i].p_status==BLOCKED) && (proctab[i].p_waitcode==event)){
	    proctab[i].p_status=RUN; 
	}

    sti();  //enable ints

}

