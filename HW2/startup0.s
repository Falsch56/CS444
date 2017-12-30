# code from Homework 2 slides
.globl _uexit
.text
	movl $0x3ffff0, %esp    # set user program stack
	call _startupc          # call C startup

_uexit: int $3			# return to tutor             

