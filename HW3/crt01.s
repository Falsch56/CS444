# file:	 crt01.s

.globl _ustart1
.text

_ustart1:
	call _main1                  #call main1 in the uprog1.c
        pushl %eax                   #push retval=exit_code on stack
        call _exit                   #sysycall exit

