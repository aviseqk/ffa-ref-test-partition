/* FF-A ABI Definitions */

#ifndef FFA_ABI_H
#define FFA_ABI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* FF-A function IDs */

#define FFA_ERROR_32			UINT32_C(0x84000060)
#define FFA_SUCCESS_32			UINT32_C(0x84000061)

#define FFA_FEATURES_32			UINT32_C(0x84000064)
#define FFA_ID_GET_32			UINT32_C(0x84000069)
#define FFA_MSG_WAIT_32			UINT32_C(0x8400006B)

#define FFA_CONSOLE_LOG_32		UINT32_C(0x8400008A)
// TODO: FFA v1.2 supports CONSOLE_LOG_64 that allows 128 characters per call, ADD THAT LATER


#define FFA_MSG_SEND_DIRECT_REQ_32	UINT32_C(0x8400006F)
#define FFA_MSG_SEND_DIRECT_RESP_32	UINT32_C(0x84000070)

/* FF-A Version */
#define FFA_VERSION_1_2			UINT32_C(0x00010002)

/* FF-A Call Registers 
 * i.e. x0 - x7
 * x0 = function id
 * x1 - x7 = argument 1 - 7
 * */

struct ffa_regs {
	uint64_t x[8];
};


/* FF-A SMC Call - C implementation */
void ffa_smc(struct ffa_regs* regs);

/* FF-A SMC Call - ASM implementation */
void ffa_smc_call(struct ffa_regs* regs);

int ffa_get_id(uint16_t *id);

/* FF-A Console Logging Helpers */
bool ffa_console_log_available(void);

bool ffa_console_puts(const char *str);

#endif
