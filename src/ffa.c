#include "ffa/ffa_abi.h"

static uint32_t negotiated_version;


void print_hex64(uint64_t value);

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

/*
 * Initialize the local FF-A library using the version
 * declared in the SP manifest.
 *
 * With manifest-configured RX/TX buffers, Hafnium maps
 * the mailbox before the SP starts executing. Consequently,
 * runtime FFA_VERSION negotiation can be rejected because
 * the FF-A framework is already in use.
 */
int ffa_init_from_manifest(uint32_t manifest_version)
{
    if (FFA_VERSION_MAJOR(manifest_version) != 1)
        return FFA_LOCAL_ERR_BAD_VERSION;

    negotiated_version = manifest_version;

    return 0;
}

int ffa_init(uint32_t requested_version)
{

	ffa_console_puts("[SP]: [DEBUG]: ffa_init() called\r\n");
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

int ffa_notification_capabilities_get(
		struct ffa_notifications_capabilities *caps)
{
	uint32_t properties = 0;
	if (!caps)
		return FFA_LOCAL_ERR_BAD_RETURN;

	*caps = (struct ffa_notifications_capabilities){0};

	caps->npi_intid = HF_INVALID_INTID;
	caps->sri_intid = HF_INVALID_INTID;

	caps->bind_status = ffa_features(
		FFA_FID_NOTIFICATION_BIND,
		0, NULL, NULL);

	caps->unbind_status = ffa_features(
		FFA_FID_NOTIFICATION_UNBIND,
		0, NULL, NULL);
	caps->set_status = ffa_features(
		FFA_FID_NOTIFICATION_SET,
		0, NULL, NULL);
	caps->get_status = ffa_features(
		FFA_FID_NOTIFICATION_GET,
		0, NULL, NULL);

	caps->npi_status = ffa_features(
		FFA_FEATURE_NOTIFICATION_PENDING_INTERRUPT,
		0, &caps->npi_intid, NULL);

	caps->sri_status = ffa_features(
		FFA_FEATURE_SCHEDULE_RECEIVER_INTERRUPT,
		0, &caps->sri_intid, NULL);

	/* NOTIFICATION_FEATURES call in FFA_FEATURES */
	caps->notification_properties_status = ffa_features(FFA_FEATURE_NOTIFICATION_FEATURES,
			0, &properties, NULL);

	if (caps->notification_properties_status == 0) {
		caps->notification_properties = properties;
		caps->vm_per_vcpu_notifications_supported =
			(properties & FFA_FEATURE_VM_PER_VCPU_NOTIFICATIONS_UNSUPPORTED) == 0;
		caps->sp_per_vcpu_notifications_supported =
			(properties & FFA_FEATURE_SP_PER_VCPU_NOTIFICATIONS_UNSUPPORTED) == 0;
		caps->vm_notification_count =
			(properties & FFA_FEATURE_NOTIFICATIONS_VM_COUNT_MASK) + FFA_FEATURE_NOTIFICATIONS_BASE_COUNT;
		caps->sp_notification_count =
			(properties & FFA_FEATURE_NOTIFICATIONS_SP_COUNT_MASK) + FFA_FEATURE_NOTIFICATIONS_BASE_COUNT;
	}
	return 0;
}

int ffa_notification_bind(uint16_t sender_id, uint16_t receiver_id,
		bool per_vcpu, uint64_t bitmap)
{
	struct ffa_regs regs = {0};

	if (!negotiated_version)
		return FFA_LOCAL_ERR_NOT_INITIALIZED;
	if (bitmap == 0)
		return FFA_LOCAL_ERR_BAD_RETURN;

	regs.x[0] = FFA_FID_NOTIFICATION_BIND;
	regs.x[1] = ((uint32_t)sender_id << 16) | (uint32_t)receiver_id;
	regs.x[2] = per_vcpu ? FFA_NOTIFICATION_FLAG_PER_VCPU : 0;
	regs.x[3] = (uint32_t)bitmap;
	regs.x[4] = (uint32_t)(bitmap >> 32);

	ffa_smc_call(&regs);

	return ffa_check_success(&regs);
}

int ffa_notification_unbind(uint16_t sender_id, uint16_t receiver_id, uint64_t bitmap)
{
	struct ffa_regs regs = {0};

	if (!negotiated_version)
		return FFA_LOCAL_ERR_NOT_INITIALIZED;
	if (bitmap == 0)
		return FFA_LOCAL_ERR_BAD_RETURN;

	regs.x[0] = FFA_FID_NOTIFICATION_UNBIND;
	regs.x[1] = ((uint32_t)sender_id << 16) | (uint32_t)receiver_id;
	regs.x[2] = 0;
	regs.x[3] = (uint32_t)bitmap;
	regs.x[4] = (uint32_t)(bitmap >> 32);

	ffa_smc_call(&regs);

	return ffa_check_success(&regs);
}

int ffa_notification_set(uint16_t sender_id, uint16_t receiver_id,
		bool per_vcpu, uint16_t receiver_vcpu_id,
		bool delay_sri, uint64_t bitmap)
{
	struct ffa_regs regs = {0};
	uint32_t flags = 0;

	if (!negotiated_version)
		return FFA_LOCAL_ERR_NOT_INITIALIZED;
	if (bitmap == 0)
		return FFA_LOCAL_ERR_BAD_RETURN;

	if (per_vcpu) {
		flags |= FFA_NOTIFICATION_FLAG_PER_VCPU;
		flags |= (uint32_t)receiver_vcpu_id << 16;
	}

	if (delay_sri)
		flags |= FFA_NOTIFICATION_FLAG_DELAY_SCHED_RECV_INTERRUPT;

	regs.x[0] = FFA_FID_NOTIFICATION_SET;
	regs.x[1] = ((uint32_t)sender_id << 16) | (uint32_t)receiver_id;
	regs.x[2] = flags;
	regs.x[3] = (uint32_t)bitmap;
	regs.x[4] = (uint32_t)(bitmap >> 32);

	ffa_smc_call(&regs);

	return ffa_check_success(&regs);
}

int ffa_notification_get(uint16_t receiver_id, uint16_t receiver_vcpu_id,
		uint32_t flags, struct ffa_notification_state *state)
{
	struct ffa_regs regs = {0};
	int ret;

	if (!negotiated_version)
		return FFA_LOCAL_ERR_NOT_INITIALIZED;
	if (!state)
		return FFA_LOCAL_ERR_BAD_RETURN;

	if ((flags & ~(0x0f)) != 0)
		return FFA_LOCAL_ERR_BAD_RETURN;

	*state = (struct ffa_notification_state){0};
	regs.x[0] = FFA_FID_NOTIFICATION_GET;
	regs.x[1] = ((uint32_t)receiver_vcpu_id << 16) | receiver_id;
	regs.x[2] = flags;

	ffa_smc_call(&regs);
	ret = ffa_check_success(&regs);
	if (ret)
		return ret;

	// FFA_NOTIFICATION_GET returns 4 notification bitmaps in 6 32-bit bitmap words
	state->sp_notifications = (uint64_t)(uint32_t)regs.x[2] |
		((uint64_t)(uint32_t)regs.x[3] << 32);
	state->vm_notifications = (uint64_t)(uint32_t)regs.x[4] |
		((uint64_t)(uint32_t)regs.x[5] << 32);
	state->spm_framework_notifications = (uint32_t)regs.x[6];
	state->hyp_framework_notifications = (uint32_t)regs.x[7];

	return 0;
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

/* FFA_RX_RELEASE implementation 
 * this call is the SP giving the ownership of its Rx buffer 
 * back to the SPMC after consuming the incoming message*/
int ffa_rx_release(void)
{
	ffa_console_puts("[SP] :[DEBUG]: ffa_rx_release() called\r\n");

	struct ffa_regs regs = {0};

	regs.x[0] = FFA_FID_RX_RELEASE;
	
	// if the SP is a secure instance, x1 is reserved
	regs.x[1] = 0;

	ffa_smc_call(&regs);

	return ffa_check_success(&regs);
}

// TODO: update the below comment
/* get the SPM framework notification bitmap
 * as the Rx-Buffer-Full Notification is a Framework Notification not an Application Notification, 
 * hence we want the SPM notification bitmap*/
int ffa_notification_get_spm_framework(
	uint16_t endpoint_id,
	uint16_t vcpu_id,
	uint32_t *bitmap)
{
	ffa_console_puts("[SP] :[DEBUG]: ffa_notification_get_spm_framework() called\r\n");

	struct ffa_regs regs = {0};

	if (!bitmap)
		return FFA_LOCAL_ERR_BAD_RETURN;

	regs.x[0] = FFA_FID_NOTIFICATION_GET;
	regs.x[1] = ((uint32_t)vcpu_id << 16) | (uint32_t)endpoint_id;	// for UP, single vCPU SP, vcpu_id is 0
	regs.x[2] = FFA_NOTIFICATION_GET_SPM_FRAMEWORK;

	ffa_smc_call(&regs);

	int ret = ffa_check_success(&regs);
	if (ret)
		return ret;

	/* as per FFA doc, in FFA_SUCCESS encoding of FFA_FID_NOTIFICATION_GET, 
	 * its' the x6 register bits [31:0] that return the SPM Framework notification bitmap
	 * basically, the bits set to identify the notifications pending for the receiver 
	 * endpoint as sent by SPM, here they are set by the SPM and tell about the notifications 
	 * regarding events in the Secure World i.e. case of our SP */
	*bitmap = (uint32_t)regs.x[6];

	return 0;
}

/**/
int ffa_notification_get_hyp_framework(
		uint16_t endpoint_id,
		uint16_t vcpu_id,
		uint32_t *bitmap)
{
	ffa_console_puts("[SP]: [DEBUG]: ffa_notification_get_hyp_framework() called\r\n");

	struct ffa_regs regs = {0};

	if (!bitmap)
		return FFA_LOCAL_ERR_BAD_RETURN;

	regs.x[0] = FFA_FID_NOTIFICATION_GET;
	regs.x[1] = ((uint32_t)vcpu_id << 16) | (uint32_t)endpoint_id;
	regs.x[2] = FFA_NOTIFICATION_GET_HYP_FRAMEWORK;

	ffa_smc_call(&regs);

	int ret = ffa_check_success(&regs);
	if (ret)
		return ret;

	/* as per FFA doc, hyp Rx buffer full notification is notification ID 32, i.e. bit0 of x7 */
	*bitmap = (uint32_t)regs.x[7];
	return 0;
}


static void ffa_copy_bytes(void *dst,
                           const void *src,
                           size_t size)
{
	uint8_t *d = dst;
	const uint8_t *s = src;

	while (size--)
		*d++ = *s++;
}

/* FFA_MSG_SEND2 implementation - the SP writing FF-A header and payload into SP's Tx Buffer */
int ffa_msg_send2(
	void *tx_buffer,
	size_t tx_buffer_size,
	uint16_t sender_id,
	uint16_t receiver_id,
	const uint8_t uuid[16],
	const void *payload,
	size_t payload_size)
{
	ffa_console_puts("[SP] :[DEBUG]: ffa_msg_send2() called\r\n");

	struct ffa_indirect_msg_header *header;
	struct ffa_regs regs = {0};

	if (!tx_buffer || !uuid || !payload)
		return FFA_LOCAL_ERR_BAD_RETURN;

	if (tx_buffer_size < sizeof(*header) || payload_size > tx_buffer_size - sizeof(*header))
		return FFA_LOCAL_ERR_BAD_RETURN;
	
	header = tx_buffer;

	header->flags = 0;
	header->reserved0 = 0;
	header->offset = sizeof(*header);

	header->sender_receiver_id = 
		(uint32_t)sender_id << 16 | (uint32_t)receiver_id;

	header->size = (uint32_t)payload_size;
	header->reserved1 = 0;

	ffa_copy_bytes(header->uuid, uuid, sizeof(header->uuid));
	ffa_copy_bytes((uint8_t *)tx_buffer + header->offset, payload, payload_size);

	/* making sure the TX buffer writes are visible before transferring control to the SPMC. */
	asm volatile("dmb ish" ::: "memory");

	regs.x[0] = FFA_FID_MSG_SEND2;
	regs.x[1] = 0;
	regs.x[2] = 0;	// msg flags: no delay-SRI request
	
	ffa_smc_call(&regs);

	return ffa_check_success(&regs);
}
