#include "ffa/ffa_abi.h"

static uint32_t negotiated_version;

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

// TODO: implement 64-bit interface of FFA_CONSOLE_LOG
bool ffa_console_log_available(void)
{
	if (!negotiated_version)
		return false;

	return ffa_function_supported(FFA_FID_CONSOLE_LOG_64);
}

/* check whether FF-A interface returned FFA_SUCCESS or FFA_ERROR 
 * */
static int ffa_check_success(struct ffa_regs *regs)
{
	uint32_t function_id = (uint32_t)regs->x[0];

	// if it was FFA_ERROR, it returns error code in x2
	if (function_id == FFA_FID_ERROR)
		return (int32_t)(uint32_t)regs->x[2];
	if (function_id != FFA_FID_SUCCESS)
		return FFA_LOCAL_ERR_BAD_RETURN;

	return 0;
}

/*
 * FFA_CONSOLE_LOG_32 uses w1/x1 as character count,
 * and 6 32-bit character registers i.e. w2-w7 for a FFA call
 * meaning, 24 characters per call, at max.
 * */
bool ffa_console_puts_32(const char *str)
{
	if (!negotiated_version || !str)
		return false;

	while (*str != '\0') {
		struct ffa_regs regs = {0};
		size_t count = 0;

		regs.x[0] = FFA_FID_CONSOLE_LOG_32;

		while (count < 24 && str[count] != '\0') {

			const unsigned int reg = 2 + count / 4;
			const unsigned int shift = (count % 4) * 8;
			
			regs.x[reg] |= (uint64_t)(uint8_t)str[count] << shift;

			count++;
		}

		regs.x[1] = count;

		ffa_smc_call(&regs);

		if (ffa_check_success(&regs) != 0)
			return false;

		str += count;
	}
	return true;
}

/*
 * FFA v1.2 supports FFA_CONSOLE_LOG_64,
 * which uses x1 as character count,
 * and 16 64-bit character registers i.e. x2-x17 for a FFA call
 * meaning, 128 characters per call, at max.
 * */
bool ffa_console_puts(const char *str)
{
	if (!negotiated_version || !str)
		return false;
	
	size_t pos = 0;

	while (str[pos] != '\0') {
		struct ffa_regs regs = {0};
		size_t count = 0;

		regs.x[0] = FFA_FID_CONSOLE_LOG_64;

		while (count < 128 && str[pos + count] != '\0') {
			uint64_t value = (uint8_t)str[pos+count];
			unsigned int reg = 2 + count / 8;
			unsigned int shift = (count % 8) * 8;
			
			regs.x[reg] |= value << shift;
			count++;
		}

		regs.x[1] = count;

		ffa_smc_call(&regs);

		if (ffa_check_success(&regs) != 0)
			return false;

		str += count;
	}
	return true;
}

int ffa_get_id(uint16_t *id)
{
	struct ffa_regs regs = {0};

	if (!negotiated_version)
		return FFA_LOCAL_ERR_NOT_INITIALIZED;

	regs.x[0] = FFA_FID_ID_GET;

	ffa_smc_call(&regs);

	int ret = ffa_check_success(&regs);
	if (ret) {
		ffa_console_puts("[SP] FFA_ID_GET failed.\r\n");
		return ret;
	}

	*id = (uint16_t)regs.x[2];

	return 0;
}



int ffa_init(uint32_t requested_version)
{
	struct ffa_regs regs = {0};
	
	if (FFA_VERSION_MAJOR(requested_version) != 1)
		return FFA_LOCAL_ERR_BAD_VERSION;

	regs.x[0] = FFA_FID_VERSION;
	regs.x[1] = requested_version;

	ffa_smc_call(&regs);

	int32_t result = (int32_t)(uint32_t)regs.x[0];

	// FFA_VERSION SMC call returns error code directly in the x0 register
	if (result < 0)
		return result;

	uint32_t remote_version = (uint32_t)result;

	if (FFA_VERSION_MAJOR(requested_version) != FFA_VERSION_MAJOR(remote_version))
		return FFA_LOCAL_ERR_BAD_VERSION;

	negotiated_version = remote_version < requested_version ? remote_version : requested_version;
	
	return 0;
}

/* to query whether a specific FF-A function is implemented at this FF-A instance */
int ffa_features(uint32_t fid, uint32_t input_properties,
		uint32_t *props_1, uint32_t *props_2)
{
	struct ffa_regs regs = {0};

	if (!negotiated_version)
		return FFA_LOCAL_ERR_NOT_INITIALIZED;

	regs.x[0] = FFA_FID_FEATURES;
	regs.x[1] = fid;
	regs.x[2] = input_properties;

	ffa_smc_call(&regs);

	int ret = ffa_check_success(&regs);

	if (ret)
		return ret;

	if (props_1)
		*props_1 = (uint32_t)regs.x[2];
	if (props_2)
		*props_2 = (uint32_t)regs.x[3];
	return 0;
}

bool ffa_function_supported(uint32_t function_id)
{
	return ffa_features(function_id, 0, NULL, NULL) == 0;
}

int ffa_rxtx_get_limits(uint32_t *min_size, uint32_t *max_size)
{
	int ret;
	uint32_t buffer_props, min_encoding, max_pages;

	if (!min_size || !max_size)
		return FFA_LOCAL_ERR_BAD_RETURN;

	ret = ffa_features(FFA_FID_RXTX_MAP_64, 0 ,&buffer_props, NULL);
	if (ret)
		return ret;

	min_encoding = (buffer_props & FFA_RXTX_MIN_SZ_MASK);
	switch (min_encoding) {
	case FFA_RXTX_MIN_SZ_4K:
		*min_size = 4 * 1024;
		break;
	case FFA_RXTX_MIN_SZ_16K:
		*min_size = 16 * 1024;
		break;
	case FFA_RXTX_MIN_SZ_64K:
		*min_size = 64 * 1024;
		break;
	default:
		return FFA_LOCAL_ERR_BAD_RETURN;
	}
	
	max_pages = (buffer_props & FFA_RXTX_MAX_PAGES_MASK) >> FFA_RXTX_MAX_PAGES_SHIFT;
	if (max_pages)
		*max_size = max_pages * FFA_RXTX_PAGE_SIZE;
	else
		*max_size = 0;
	
	return 0;
}
