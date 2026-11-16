.section .text.ffa_smc, "ax"
.align 4

.globl ffa_smc_call
.type ffa_smc_call, %function

/* 
  void ffa_smc_call(struct ffa_regs* regs); 

  register x0 is the pointer to struct ffa_regs regs
  regs->x[0] .... regs->x[7] become SMC arguments,
  and then returned x0->x7 are written back to regs
*/
ffa_smc_call:
	/* Step 1: */
	sub sp, sp, #16
	str x0, [sp, #0]
	str x30, [sp, #8]

	/* Step 2: */
	mov x8, x0

	/* Step 3: */
	ldp x0, x1, [x8, #0]
	ldp x2, x3, [x8, #16]
	ldp x4, x5, [x8, #32]
	ldp x6, x7, [x8, #48]

	/* Step 4: */
	smc #0

	/* Step 5: */
	ldr x8, [sp, #0]

	/* Step 6: */
	stp x0, x1, [x8, #0]
	stp x2, x3, [x8, #16]
	stp x4, x5, [x8, #32]
	stp x6, x7, [x8, #48]

	/* Step 7: */
	ldr x30, [sp, #8]
	add sp, sp, #16

	ret

.size ffa_smc_call, . - ffa_smc_call
