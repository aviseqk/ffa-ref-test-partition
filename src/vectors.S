.section .vectors, "ax"

.balign 2048

.macro VECTOR target 
	b \target
	.space 124
.endm	

.globl sp_vectors
.type sp_vectors, %object

sp_vectors:

	/* Current EL with SP0 */
	VECTOR sp_unhandled_exception	/* Sync */
	VECTOR sp_unhandled_exception	/* IRQ */
	VECTOR sp_unhandled_exception	/* FIQ */
	VECTOR sp_unhandled_exception	/* SError */

	/* Current EL with SPx -> this SP's S-EL1 execution context */
	VECTOR sp_unhandled_exception	/* Sync */
	VECTOR sp_irq_exception		/* IRQ */
	VECTOR sp_unhandled_exception	/* FIQ */
	VECTOR sp_unhandled_exception	/* SError */

	/* Lower EL using AArch64 */
	VECTOR sp_unhandled_exception	/* Sync */
	VECTOR sp_unhandled_exception	/* IRQ */
	VECTOR sp_unhandled_exception	/* FIQ */
	VECTOR sp_unhandled_exception	/* SError */

	/* Lower EL using AArch32 */
	VECTOR sp_unhandled_exception	/* Sync */
	VECTOR sp_unhandled_exception	/* IRQ */
	VECTOR sp_unhandled_exception	/* FIQ */
	VECTOR sp_unhandled_exception	/* SError */

.size sp_vectors, . - sp_vectors

.section .text.exceptions, "ax"

.align 4

/* 
	IRQ Exception Entry
	we preserve the GPR state when interrupted, call C handler,
	restore the preserved interrupted state and return with ERET to SPMC
*/
sp_irq_exception:

	sub sp, sp, #256

	stp x0,  x1,  [sp, #0]
	stp x2,  x3,  [sp, #16]
	stp x4,  x5,  [sp, #32]
	stp x6,  x7,  [sp, #48]
	stp x8,  x9,  [sp, #64]
	stp x10, x11, [sp, #80]
	stp x12, x13, [sp, #96]
	stp x14, x15, [sp, #112]
	stp x16, x17, [sp, #128]
	stp x18, x19, [sp, #144]
	stp x20, x21, [sp, #160]
	stp x22, x23, [sp, #176]
	stp x24, x25, [sp, #192]
	stp x26, x27, [sp, #208]
	stp x28, x29, [sp, #224]
	str x30,      [sp, #240]

	bl sp_irq_handler

	ldp x0,  x1,  [sp, #0]
	ldp x2,  x3,  [sp, #16]
	ldp x4,  x5,  [sp, #32]
	ldp x6,  x7,  [sp, #48]
	ldp x8,  x9,  [sp, #64]
	ldp x10, x11, [sp, #80]
	ldp x12, x13, [sp, #96]
	ldp x14, x15, [sp, #112]
	ldp x16, x17, [sp, #128]
	ldp x18, x19, [sp, #144]
	ldp x20, x21, [sp, #160]
	ldp x22, x23, [sp, #176]
	ldp x24, x25, [sp, #192]
	ldp x26, x27, [sp, #208]
	ldp x28, x29, [sp, #224]
	ldr x30,      [sp, #240]

	add sp, sp, #256

	eret

/* NOTE: This unhandled exception and the masking of interrupts 
was leading to a infinite loop when SPMC sent a vIRQ post MSG_SEND2 call, 
TODO: make an extensive noting of this issue */
sp_unhandled_exception:

	msr daifset, #0xf

1:
	wfe
	b 1b
