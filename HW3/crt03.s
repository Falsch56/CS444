# file:	 crt03.s

.globl _ustart3
.text

_ustart3:
	call _main3                  #call main3 in the uprog3.c
        pushl %eax                   #push retval=exit_code on stack
        call _exit                   #sysycall exit

