#include "ffa/ffa_abi.h"

/* NOTE: this is a SMC call, but written in C with inline assembly approach,
 * there is also a pure assembly wrapper written for the same purpose, in `src/ffa_smc.S` 
 * which does not have to consider a lot of compiler adjustements about different registers
 * that might not remain the same post an SMC call */
void ffa_smc(struct ffa_regs *regs)
{
	register uint64_t x0 asm("x0") = regs->x[0];
	register uint64_t x1 asm("x1") = regs->x[1];
	register uint64_t x2 asm("x2") = regs->x[2];
	register uint64_t x3 asm("x3") = regs->x[3];
	register uint64_t x4 asm("x4") = regs->x[4];
	register uint64_t x5 asm("x5") = regs->x[5];
	register uint64_t x6 asm("x6") = regs->x[6];
	register uint64_t x7 asm("x7") = regs->x[7];

	asm volatile(
			"smc #0" :
			"+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3),
			"+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7)
			:
			:
			"x8", "x9", "x10", "x11", "x12", "x13", "x14",
			"x15", "x16", "x17", "cc", "memory"
			);


	regs->x[0] = x0;
	regs->x[1] = x1;
	regs->x[2] = x2;
	regs->x[3] = x3;
	regs->x[4] = x4;
	regs->x[5] = x5;
	regs->x[6] = x6;
	regs->x[7] = x7;
}

bool ffa_console_log_available(void)
{
	struct ffa_regs regs = {0};

	regs.x[0] = FFA_FEATURES_32;
	regs.x[1] = FFA_CONSOLE_LOG_32;

	ffa_smc(&regs);

	return regs.x[0] == FFA_SUCCESS_32;
}

/*
 * FFA_CONSOLE_LOG_32 uses w1/x1 as character count,
 * and 6 32-bit character registers i.e. w2-w7 for a FFA call
 * meaning, 24 characters per call, at max.
 * */
// TODO: FFA v1.2 supports CONSOLE_LOG_64 that allows 128 characters per call, ADD THAT LATER
bool ffa_console_puts(const char *str)
{
	while (*str != '\0') {
		struct ffa_regs regs = {0};
		size_t count = 0;

		regs.x[0] = FFA_CONSOLE_LOG_32;

		while (count < 24 && str[count] != '\0') {

			const unsigned int reg = 2 + count / 4;
			const unsigned int shift = (count % 4) * 8;
			
			regs.x[reg] |= (uint64_t)(uint8_t)str[count] << shift;

			count++;
		}

		regs.x[1] = count;

		ffa_smc(&regs);

		if (regs.x[0] != FFA_SUCCESS_32)
			return false;

		str += count;
	}
	return true;

}

int ffa_get_id(uint16_t *id)
{
	struct ffa_regs regs = {0};

	regs.x[0] = FFA_ID_GET_32;

	ffa_smc_call(&regs);

	if ((uint32_t)regs.x[0] != FFA_SUCCESS_32) {
		ffa_console_puts("[SP] FFA_ID_GET failed.\r\n");
		return -1;
	}

	*id = (uint16_t)regs.x[2];

	return 0;
}
