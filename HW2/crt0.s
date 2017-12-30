# Taken from class notes
.global _ustart
.text

_ustart:	movl $0x3ffff0, %esp  # init the stack
		movl $0, %ebp         # init pointer
		call _main            # the main in test1.c
		pushl %eax            # push exit code value to stack
		call _exit	      # syscall exit
