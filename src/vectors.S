.section .vectors, "ax"

.balign 2048

.globl sp_vectors
.type sp_vectors, %object

sp_vectors:

	.rept 16
		b sp_unhandled_exception
		.space 124
	.endr

.size sp_vectors, . - sp_vectors

.section .text.exceptions, "ax"

.align 4

sp_unhandled_exception:

	msr daifset, #0xf

1:
	wfe
	b 1b
