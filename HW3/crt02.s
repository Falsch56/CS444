# file:	 crt02.s

.globl _ustart2
.text

_ustart2:
	call _main2                  #call main2 in the uprog2.c
        pushl %eax                   #push retval=exit_code on stack
        call _exit                   #sysycall exit

