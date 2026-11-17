.section .text.ffa_smc, "ax"
.align 4

.globl ffa_smc_call
.type ffa_smc_call, %function

/* 
  void ffa_smc_call(struct ffa_regs* regs); 

  register x0 is the pointer to struct ffa_regs regs
  regs->x[0] .... regs->x[17] become SMC arguments,
  then the "smc #0" call, 
  and then returned x0->x17 are copied back to regs

  The regs pointer, saved callee state, and the LR of callee is saved on stack as a frame
*/
ffa_smc_call:
	/* Step 1: allocate 32-byte stack frame for the state saving */
	sub sp, sp, #32

	str x0, [sp, #0]
	str x19, [sp, #8]
	str x30, [sp, #16]


	/* Step 2: preserve the caller's x19 register,
	x19 is the temporary register used as base pointer, hence saved on stack as well */
	mov x19, x0

	/* Step 3: load SMC arguments into registers */
	ldp x0, x1, [x19, #0]
	ldp x2, x3, [x19, #16]
	ldp x4, x5, [x19, #32]
	ldp x6, x7, [x19, #48]
	ldp x8, x9, [x19, #64]
	ldp x10, x11, [x19, #80]
	ldp x12, x13, [x19, #96]
	ldp x14, x15, [x19, #112]
	ldp x16, x17, [x19, #128]

	/* Step 4: call into SPMC using the "smc" conduit */
	smc #0

	/* Step 5: restore the pointer */
	ldr x19, [sp, #0]

	/* Step 6: write returned registers back into the struct regs */
	stp x0, x1, [x19, #0]
	stp x2, x3, [x19, #16]
	stp x4, x5, [x19, #32]
	stp x6, x7, [x19, #48]
	stp x8, x9, [x19, #64]
	stp x10, x11, [x19, #80]
	stp x12, x13, [x19, #96]
	stp x14, x15, [x19, #112]
	stp x16, x17, [x19, #128]

	/* Step 7: restore caller state */
	ldr x19, [sp, #8]
	ldr x30, [sp, #16]

	add sp, sp, #32

	ret

.size ffa_smc_call, . - ffa_smc_call
