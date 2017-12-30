#include <cpu.h>
#include <gates.h>
#include "tsyscall.h"
#include "tsystm.h"

extern IntHandler syscall;
extern void locate_idt(unsigned int *limitp, char ** idtp); 
extern void ustart(void);
extern void uexit(void);
extern void debug_log(char *msg);	

//procedures defined in this file
void k_init(void);
void set_trap_gate(int n, IntHandler *inthand_addr);
void syscallc( int user_eax, int devcode, char *buff , int bufflen);
int sysexit(int code);

void k_init(){
  ioinit();            
  set_trap_gate(0x80, &syscall);			
  ustart();
}

//sets trap gate and fills in desc table
void set_trap_gate(int n, IntHandler *inthand_addr){
  
  char *idt_addrLinear;
  char *idt_addrVirtual;
  Gate_descriptor *idt, *desc;
  unsigned int val = 0;

  locate_idt(&val,&idt_addrLinear);
  idt_addrVirtual = idt_addrLinear - KERNEL_BASE_LA;

  idt = (Gate_descriptor *) idt_addrVirtual;
  desc = &idt[n];

  //Fill in the descriptor table
  desc->selector = KERNEL_CS;   
  desc->addr_hi = ((unsigned int)inthand_addr)>>16; 
  desc->addr_lo = ((unsigned int)inthand_addr)&0xffff;
  desc->flags = GATE_P|GATE_DPL_KERNEL|GATE_TRAPGATE; 
  desc->zero = 0;
}

//Program to select trap statement to execute
void syscallc( int user_eax, int devcode, char *buff , int bufflen)
{
  int trap = user_eax;

  switch(trap){
	case 1:
		sysexit( 0 );
		break;
	case 2:
		sysread(devcode, buff, bufflen);
		break;
	case 3:
		syswrite(devcode, buff, bufflen);
		break;
	default :
		kprintf("eax val does not reference a trap-call");
	}
    }

//Function to print log and exit program
int sysexit(int code){ 
        kprintf("\n EXIT CODE IS %d\n", code);
	kprintf("SHUTTING THE SYSTEM DOWN!\n");
    	kprintf("Debug log from run:\n");
    	kprintf("Marking kernel events as follows:\n");
    	kprintf("  ^a   COM2 input interrupt, a received\n");
    	kprintf("  ~    COM2 output interrupt, ordinary char output\n");
    	kprintf("  ~e   COM2 output interrupt, echo output\n");
   	kprintf("  ~s   COM2 output interrupt, shutdown TX ints\n");
    	kprintf("user start<><>");
    	kprintf("\nLEAVE KERNEL!\n\n");
    	uexit();
	return 0;    
}

