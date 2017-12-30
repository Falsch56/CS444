# Code from Homework 2 slides
.globl _syscallc, _syscall      

_syscall:       pushl %edx       #push value to edx stack 
                pushl %ecx	 # arg1
                pushl %ebx 	 # arg2
                pushl %eax	 # arg3
                call _syscallc   # call c trap routine in tunix.c
                addl $4, %esp    # add 4 to skip over eax from stack
                popl %ebx        # pop values of ebx to edx from stack 
                popl %ecx        # ^ 
                popl %edx	 # ^
                iret

