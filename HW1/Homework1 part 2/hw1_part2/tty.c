/*********************************************************************
*
*       file:           tty.c
*       author:         betty o'neil
*
*       tty driver--device-specific routines for ttys 
*
*/
#include <stdio.h>  /* for kprintf prototype */
#include <serial.h>
#include <cpu.h>
#include <pic.h>
#include "ioconf.h"
#include "tty_public.h"
#include "tty.h"

#define MAX 6

struct tty ttytab[NTTYS];        /* software params/data for each SLU dev */

/* tell C about the assembler shell routines */
extern void irq3inthand(void), irq4inthand(void);

/* C part of interrupt handlers--specific names called by the assembler code */
extern void irq3inthandc(void), irq4inthandc(void); 

/* the common code for the two interrupt handlers */                           static void irqinthandc(int dev); 

/*====================================================================
*
*       tty specific initialization routine for COM devices
*
*/

void ttyinit(int dev)
{
  int baseport;
  struct tty *tty;		/* ptr to tty software params/data block */

  baseport = devtab[dev].dvbaseport; /* pick up hardware addr */
  tty = (struct tty *)devtab[dev].dvdata; /* and software params struct */

  if (baseport == COM1_BASE) {
      /* arm interrupts by installing int vec */
      set_intr_gate(COM1_IRQ+IRQ_TO_INT_N_SHIFT, &irq4inthand);
      pic_enable_irq(COM1_IRQ);
  } else if (baseport == COM2_BASE) {
      /* arm interrupts by installing int vec */
      set_intr_gate(COM2_IRQ+IRQ_TO_INT_N_SHIFT, &irq3inthand);
      pic_enable_irq(COM2_IRQ);
  } else {
      kprintf("Bad TTY device table entry, dev %d\n", dev);
      return;			/* give up */
  }
  tty->echoflag = 1;		/* default to echoing */
  
  init_queue(&(tty->qin), MAX);    //init in queue
  init_queue(&(tty->qout), MAX);   //init out queue
  init_queue(&(tty->qecho), MAX);  //init echo queue

  /* enable interrupts on receiver */
  outpt(baseport+UART_IER, UART_IER_RDI); /* RDI = receiver data int */
}


/*====================================================================
*
*       Useful function when emptying/filling the read/write buffers
*
*/

#define min(x,y) (x < y ? x : y)


/*====================================================================
*
*       tty-specific read routine for TTY devices
*
*/

int ttyread(int dev, char *buf, int nchar)
{
  int baseport;
  struct tty *tty;
  int i;
  int saved_eflags;        /* old cpu control/status reg, so can restore it */

  baseport = devtab[dev].dvbaseport; /* hardware addr from devtab */
  tty = (struct tty *)devtab[dev].dvdata;   /* software data for line */

   for (i = 0; i < nchar; ) { 
	saved_eflags = get_eflags();             //get state before disallow interrupts
	cli();	                                 //disallow interrupts
				
	if ( queuecount( &(tty->qin)) != 0 ){    //if queuecount does not equal zero, we want to dequeue
	    buf[i] = dequeue(&(tty->qin));       //read characters into buffer from devtab
            i++;                                 //increment
	}

	set_eflags(saved_eflags);                //return to previous state
    }


  return nchar;       
}


/*====================================================================
*
*       tty-specific write routine for SAPC devices
*       (cs444: note that the buffer tbuf is superfluous in this code, but
*        it still gives you a hint as to what needs to be done for
*        the interrupt-driven case)
*
*/

int ttywrite(int dev, char *buf, int nchar)
{
  int baseport;
  struct tty *tty;
  int i;
  int saved_eflags;
  int out = UART_IER_RDI | UART_IER_THRI;

  baseport = devtab[dev].dvbaseport; /* hardware addr from devtab */
  tty = (struct tty *)devtab[dev].dvdata;   /* software data for line */

  saved_eflags = get_eflags();      //save eflags before disallow interrupts
  cli();		            //disallow interrupts. Interfered with print i/o
    
  //Put some chars into qout  
  for (i = 0; (i < nchar) && (enqueue( &(tty->qout), buf[i])!=FULLQUE) ; )
    i++;

  outpt( baseport+UART_IER, out);    //start interrput and restart output
  set_eflags(saved_eflags);          //restore
    
  //Loop until all characters are in queue
  for (; i < nchar ;) {
      cli();			

      if (enqueue( &(tty->qout), buf[i]) != FULLQUE) {
	    i++;
	    outpt( baseport+UART_IER, out);  //allow for transmit interrupts
 
	}
	set_eflags(saved_eflags); 
    }


  return nchar;

}

/*====================================================================
*
*       tty-specific control routine for TTY devices
*
*/

int ttycontrol(int dev, int fncode, int val)
{
  struct tty *this_tty = (struct tty *)(devtab[dev].dvdata);

  if (fncode == ECHOCONTROL)
    this_tty->echoflag = val;
  else return -1;
  return 0;
}



/*====================================================================
*
*       tty-specific interrupt routine for COM ports
*
*   Since interrupt handlers don't have parameters, we have two different
*   handlers.  However, all the common code has been placed in a helper 
*   function.
*/
  
void irq4inthandc()
{
  irqinthandc(TTY0);
}                              
  
void irq3inthandc()
{
  irqinthandc(TTY1);
}                              

void irqinthandc(int dev){  
  
  int ch, status;
  int out = UART_IER_RDI | UART_IER_THRI;  //variable to hold common code
  struct tty *tty = (struct tty *)(devtab[dev].dvdata);
  int baseport = devtab[dev].dvbaseport; /* hardware i/o port */;


  pic_end_int();                 //End pic 
  status = inpt(baseport+UART_LSR);  //set a status line to determine how we go forward


  if (status & UART_LSR_DR) {  //Read-only mode
    ch = inpt(baseport+UART_RX);
    if (queuecount(&tty->qin) < MAX) {
      enqueue( &tty->qin, ch );
      if (tty->echoflag) {
        enqueue(&tty->qecho , ch);
        outpt( baseport+UART_IER, out);
      }
    }
  }

 
  if (status & UART_LSR_THRE) { //Write-mode
    if (queuecount(&tty->qecho)) { //if queuecount for qecho is not empty
      outpt( baseport+UART_TX, dequeue( &tty->qecho ) ); 
    }
    else if (queuecount(&tty->qout) != 0) { //if queuecount for qout is not empty
      ch = dequeue(&tty->qout);             
      outpt( baseport+UART_TX, ch);
      outpt(baseport+UART_IER, UART_IER_RDI);
      outpt(baseport+UART_IER, out);
    }
    else {
      outpt( baseport+UART_IER, UART_IER_RDI); 
    }
  }
}
