/* FF-A ABI Definitions */

#ifndef FFA_ABI_H
#define FFA_ABI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Generic FF-A function IDs */
#define FFA_FID_ERROR			UINT32_C(0x84000060)
#define FFA_FID_SUCCESS			UINT32_C(0x84000061)
#define FFA_FID_INTERRUPT		UINT32_C(0x84000062)
#define FFA_FID_VERSION			UINT32_C(0x84000063)
#define FFA_FID_FEATURES		UINT32_C(0x84000064)

/* FF-A Version */
#define FFA_VERSION_1_0			UINT32_C(0x00010000)
#define FFA_VERSION_1_1			UINT32_C(0x00010001)
#define FFA_VERSION_1_2			UINT32_C(0x00010002)
#define FFA_VERSION_1_3			UINT32_C(0x00010003)

#define FFA_VERSION_MAJOR(v)		(((v) >> 16) & 0x7fff)
#define FFA_VERSION_MINOR(v)		((v) & 0xffff)

/* Discovery FF-A function IDs */
#define FFA_FID_PARTITION_INFO_GET	UINT32_C(0x84000068)
#define FFA_FID_ID_GET			UINT32_C(0x84000069)
#define FFA_FID_SPM_ID_GET		UINT32_C(0x84000085)
#define FFA_FID_PARTITION_INFO_GET_REGS_64 \
					UINT32_C(0xC400008B)

/* Scheduling - Execution FF-A function IDs */
#define FFA_FID_MSG_POLL		UINT32_C(0x8400006A)
#define FFA_FID_MSG_WAIT		UINT32_C(0x8400006B)
#define FFA_FID_YIELD			UINT32_C(0x8400006C)
#define FFA_FID_RUN			UINT32_C(0x8400006D)

/* Buffer management FF-A function IDs */
#define FFA_FID_RX_RELEASE		UINT32_C(0x84000065)

#define FFA_FID_RXTX_MAP_32		UINT32_C(0x84000066)
#define FFA_FID_RXTX_MAP_64		UINT32_C(0xC4000066)

#define FFA_FID_RXTX_UNMAP		UINT32_C(0x84000067)

/* Messaging FF-A function IDs */
#define FFA_FID_MSG_SEND		UINT32_C(0x8400006E)

#define FFA_FID_MSG_SEND_DIRECT_REQ_32	UINT32_C(0x8400006F)
#define FFA_FID_MSG_SEND_DIRECT_RESP_32	UINT32_C(0x84000070)

#define FFA_FID_MSG_SEND_DIRECT_REQ2_64	UINT32_C(0xC400008D)
#define FFA_FID_MSG_SEND_DIRECT_RESP2_64 \
					UINT32_C(0xC400008E)
#define FFA_FID_MSG_SEND2		UINT32_C(0x84000086)

/* Memory Management FF-A function IDs */
#define FFA_FID_MEM_DONATE_32		UINT32_C(0x84000071)
#define FFA_FID_MEM_DONATE_64		UINT32_C(0xC4000071)

#define FFA_FID_MEM_LEND_32		UINT32_C(0x84000072)
#define FFA_FID_MEM_LEND_64		UINT32_C(0xC4000072)

#define FFA_FID_MEM_SHARE_32		UINT32_C(0x84000073)
#define FFA_FID_MEM_SHARE_64		UINT32_C(0xC4000073)

#define FFA_FID_MEM_RETRIEVE_REQ_32	UINT32_C(0x84000074)
#define FFA_FID_MEM_RETRIEVE_REQ_64	UINT32_C(0xC4000074)

#define FFA_FID_MEM_RETRIEVE_RESP	UINT32_C(0x84000075)
#define FFA_FID_MEM_RELINQUISH		UINT32_C(0x84000076)
#define FFA_FID_MEM_RECLAIM		UINT32_C(0x84000077)

#define FFA_FID_MEM_OP_PAUSE		UINT32_C(0x84000078)
#define FFA_FID_MEM_OP_RESUME		UINT32_C(0x84000079)

#define FFA_FID_MEM_FRAG_RX		UINT32_C(0x8400007A)
#define FFA_FID_MEM_FRAG_TX		UINT32_C(0x8400007B)


/* Notifications FF-A function IDs */
#define FFA_FID_NOTIFICATION_BITMAP_CREATE \
					UINT32_C(0x8400007D)

#define FFA_FID_NOTIFICATION_BITMAP_DESTROY \
					UINT32_C(0x8400007E)

#define FFA_FID_NOTIFICATION_BIND	UINT32_C(0x8400007F)
#define FFA_FID_NOTIFICATION_UNBIND	UINT32_C(0x84000080)
#define FFA_FID_NOTIFICATION_SET	UINT32_C(0x84000081)
#define FFA_FID_NOTIFICATION_GET	UINT32_C(0x84000082)

#define FFA_FID_NOTIFICATION_INFO_GET_32 \
					UINT32_C(0x84000083)

#define FFA_FID_NOTIFICATION_INFO_GET_64 \
					UINT32_C(0xC4000083)


/* Miscellaneous FF-A interfaces function IDs */
#define FFA_FID_CONSOLE_LOG_32		UINT32_C(0x8400008A)
#define FFA_FID_CONSOLE_LOG_64		UINT32_C(0xC400008A)

#define FFA_FID_SECONDARY_EP_REGISTER \
					UINT32_C(0x84000087)

#define FFA_FID_MEM_PERM_GET_64		UINT32_C(0xC4000088)
#define FFA_FID_MEM_PERM_SET_64		UINT32_C(0xC4000089)

/* FF-A Return Codes */
#define FFA_RET_SUCCESS			0
#define FFA_RET_NOT_SUPPORTED		(-1)
#define FFA_RET_INVALID_PARAMETERS	(-2)
#define FFA_RET_NO_MEMORY		(-3)
#define FFA_RET_BUSY			(-4)
#define FFA_RET_INTERRUPTED		(-5)
#define FFA_RET_DENIED			(-6)
#define FFA_RET_RETRY			(-7)
#define FFA_RET_ABORTED			(-8)
#define FFA_RET_NO_DATA			(-9)
#define FFA_RET_NOT_READY		(-10)

/* FFA_RXTX_MAP function properties */
#define FFA_RXTX_PAGE_SIZE		4096U
#define FFA_RXTX_MIN_SZ_MASK		UINT32_C(0x3)
#define FFA_RXTX_MAX_PAGES_MASK		UINT32_C(0xffff0000)
#define FFA_RXTX_MAX_PAGES_SHIFT	16

#define FFA_RXTX_MIN_SZ_4K		0x0
#define FFA_RXTX_MIN_SZ_64K		0x1
#define FFA_RXTX_MIN_SZ_16K		0x2

/* Local Runtime Errors */
#define FFA_LOCAL_ERR_NOT_INITIALIZED   (-1000)
#define FFA_LOCAL_ERR_BAD_VERSION       (-1001)
#define FFA_LOCAL_ERR_BAD_RETURN	(-1002)

/* FF-A Call Registers for SMC calls
 * for FF-A 1.2 direct msg as it uses x0 - x17
 * */
#define FFA_REG_COUNT 18
struct ffa_regs {
	uint64_t x[FFA_REG_COUNT];
};


/* FF-A SMC Call - C implementation */
void ffa_smc(struct ffa_regs* regs);

/* FF-A SMC Call - ASM implementation */
void ffa_smc_call(struct ffa_regs* regs);

int ffa_get_id(uint16_t *id);

/* FF-A Console Logging Helpers */
bool ffa_console_log_available(void);

bool ffa_console_puts(const char *str);
bool ffa_console_puts_32(const char *str);

int ffa_init(uint32_t requested_version);

int ffa_features(uint32_t fid, uint32_t input_properties,
		uint32_t *props_1, uint32_t *props_2);

bool ffa_function_supported(uint32_t function_id);

int ffa_rxtx_get_limits(uint32_t *min_size, uint32_t* max_size);

#endif
