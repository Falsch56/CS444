//Test Program adapted from Testio.c

#include "stdio.h"
#include "tunistd.h"
#include "tty_public.h"

#define MILLION 1000000
#define DELAYLOOPCOUNT (1000000000)
#define BUFLEN 80


void delay(void);

int main(void){	
  char buf[BUFLEN];
  int got, i, ldev;
  ldev = TTY1;
  
  kprintf("\nTrying simple write(4 chars)...\n");
  got = write(ldev,"hi\n",4);
  kprintf("write of 4 returned %d\n", got);
  delay();

  kprintf("Trying longer write (9 chars)\n");
  got = write(ldev, "abcdefghi", 9);
  kprintf("write of 9 returned %d\n",got);
  delay(); 

  for (i = 0; i < BUFLEN; i++)
      buf[i] = 'A'+ i/2;
  kprintf("\nTrying write of %d-char string...\n", BUFLEN);
  got = write(ldev, buf, BUFLEN);
  kprintf("\nwrite returned %d\n", got);
  delay();
 
   
  
  kprintf("\nTrying Read...\n");
  read(ldev, buf, 10);
  kprintf("\nReturned from read, trying write of buf...\n");
  write(ldev, buf, 10);
  return 0;
  }


void delay(){
  
  int i;
  for (i = 0; i < DELAYLOOPCOUNT; i++)
	;
}

