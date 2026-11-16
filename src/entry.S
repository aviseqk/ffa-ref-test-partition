.section .text.boot, "ax"

.align 4

.globl _start
.type _start, %function

_start:
	msr daifset, #0xf

	/*clear the bss region*/
	adrp x0, __bss_start
	add x0, x0, :lo12:__bss_start

	adrp x1, __bss_end
	add x1, x1, :lo12:__bss_end

1:
	cmp x0, x1
	b.hs 2f
	
	str xzr, [x0], #8
	b 1b

2:
	/* initialize S-EL1 stack */
	adrp x0, __stack_top
	add x0, x0, :lo12:__stack_top

	mov sp, x0

	/* setup the exception vector table */
	adrp x0, sp_vectors
	add  x0, x0, :lo12:sp_vectors

	msr VBAR_EL1, x0
	isb

	bl sp_main

3:
	wfe
	b 3b

.size _start, . - _start
