#include "ffa/ffa_abi.h"
#include <stdint.h>

/* iur SP endpoint ID */
#define SP_ENDPOINT_ID		0x8001

/* Linux normal-world endpoint ID. */
#define LINUX_ENDPOINT_ID	0x0000

const uint8_t SP_UUID_BYTES[16] = {
	0xf2, 0x50, 0x79, 0x3f,
	0xe5, 0x40,
	0x09, 0x7a,
	0xb9, 0x68, 0x83, 0xbd,
	0x7d, 0xa0, 0xbb, 0x4c
};

/* our SP's UUID */
#define SP_UUID_LO		UINT64_C(0x7a0940e53f7950f2)
#define SP_UUID_HI		UINT64_C(0x4cbba07dbd8368b9)

/* NPI ID as discovered through FFA_FEATURES at startup. */
static uint32_t sp_npi_intid = HF_INVALID_INTID;

/* Last Virtual Interrupt handled by the SP */
volatile uint32_t sp_last_irq_id = HF_INVALID_INTID;

void sp_irq_handler(void)
{
	// retrieve and acknowledge this pending virtual interrupt using the 
	// hafnium's hvc interface `HF_INTERRUPT_GET`
	sp_last_irq_id = hf_interrupt_get();
}

/* Application protocol. - for 32 bit Direct Messaging */
#define REQUEST_MAGIC		UINT32_C(0x4c4e5831) /* LNX1 */
#define RESPONSE_MAGIC		UINT32_C(0x53505231) /* SPR1 */

#define OPCODE_XOR		UINT32_C(1)
#define REQUEST_NONCE		UINT32_C(0x1234abcd)
#define XOR_MASK		UINT32_C(0xa5a5a5a5)
#define RESPONSE_TRAILER	UINT32_C(0x53504f4e) /* SPON */

/* Application protocol. - for 32 bit Direct Messaging v2 */
#define REQUEST_MAGIC_v2	UINT32_C(0x4c4e5832) /* LNX2 */
#define RESPONSE_MAGIC_v2	UINT32_C(0x53505232) /* SPR2 */
#define XOR_MASK_v2		UINT64_C(0xa5a5a5a5a5a5a5a5)
#define REQUEST_NONCE_v2	UINT32_C(0x1234abcd)
#define RESPONSE_TRAILER_v2	UINT32_C(0x53504f4e) /* SPON */

/* Protocol response status codes. */

#define SP_STATUS_OK		0
#define SP_STATUS_BAD_SENDER	1
#define SP_STATUS_BAD_RECEIVER	2
#define SP_STATUS_BAD_FLAGS	3
#define SP_STATUS_BAD_MAGIC	4
#define SP_STATUS_BAD_OPCODE    5
#define SP_STATUS_BAD_RESERVED  6
#define SP_STATUS_WRONG_UUID	7

void print_hex32(uint32_t value);
void print_hex16(uint16_t value);
void print_hex64(uint64_t value);

/* INFO: RELOCATION CODE Issue: refer to `notes/relocation-bug-using-abs-pointers-in-sp.md` for note
 * on the problem with using pointers to string literals for ffa_function names*/
struct ffa_function {
	const char name[32];
	uint32_t fid;
	//bool need_input_prop;
};

#define ARRAY_SIZE(arr)		(sizeof(arr) / sizeof((arr)[0]))

static const struct ffa_function functions_to_check[] = {
	{"FFA_CONSOLE_LOG_32", FFA_FID_CONSOLE_LOG_32},
	{"FFA_CONSOLE_LOG_64", FFA_FID_CONSOLE_LOG_64},
	{"FFA_DIRECT_REQ2", FFA_FID_MSG_SEND_DIRECT_REQ2_64},
	{"FFA_DIRECT_RESP2", FFA_FID_MSG_SEND_DIRECT_RESP2_64},
	{"FFA_DIRECT_REQ", FFA_FID_MSG_SEND_DIRECT_REQ_32},
	{"FFA_DIRECT_RESP", FFA_FID_MSG_SEND_DIRECT_RESP_32},
	{"FFA_RXTX_MAP_32", FFA_FID_RXTX_MAP_32},
	{"FFA_MSG_SEND2", FFA_FID_MSG_SEND2},
	{"FFA_NOTIFICATION_BIND", FFA_FID_NOTIFICATION_BIND},
	{"FFA_NOTIFICATION_SET", FFA_FID_NOTIFICATION_SET},
	{"FFA_NOTIFICATION_GET", FFA_FID_NOTIFICATION_GET},
	//{"FFA_MEM_RETRIEVE_REQ2", FFA_FID_MEM_RETRIEVE_REQ_64},
	//{"FFA_RXTX_MAP_64", FFA_FID_RXTX_MAP_64},
};

static void report_ffa_function(const char *name, uint32_t fid)
{
	ffa_console_puts(name);
	
	if (ffa_function_supported(fid))
		ffa_console_puts(": supported\n");
	else
		ffa_console_puts(": NOT supported\n");
}

/* making SP report all the available FFA capabilites, post FFA_FEATURES call */
static void report_all_ffa_functions_check(const struct ffa_function *functions, size_t count)
{
	ffa_console_puts("\r\n[SP] FF-A capabilities Report\r\n");

	for (size_t i = 0; i < count; i++) {
		report_ffa_function(
				functions[i].name,
				functions[i].fid);
	}

	// NOTE: this feature checked differently as it needs input props in FFA_FEATURES call
	ffa_console_puts("FFA_FID_MEM_RETRIEVE_REQ_64");
	
	int ret = ffa_features(FFA_FID_MEM_RETRIEVE_REQ_64, 0x2, NULL, NULL);
	if (ret == 0)
		ffa_console_puts(": supported\n");
	else
		ffa_console_puts(": NOT supported\n");


	ffa_console_puts("FFA_RXTX_MAP_64");
	uint32_t buffer_props;

	ret = ffa_features(FFA_FID_RXTX_MAP_64, 0 ,&buffer_props, NULL);
	if (ret == 0) {
		ffa_console_puts(": supported\n");
	}
	else
		ffa_console_puts(": NOT supported\n");
}

#define NOTIF_LINUX_TO_SP_ID		19U
#define NOTIF_SP_TO_LINUX_ID		20U
#define LINUX_TO_SP_NOTIF_BIT		(UINT64_C(1) << NOTIF_LINUX_TO_SP_ID)

static void report_notifications_features(
		const struct ffa_notifications_capabilities *caps)
{
	ffa_console_puts("[SP] Notification Feature Capability Report\r\n");
	ffa_console_puts("[SP] FFA_NOTIFICATION_BIND: ");
	print_hex32((uint32_t)caps->bind_status);
	ffa_console_puts("[SP] FFA_NOTIFICATION_UNBIND: ");
	print_hex32((uint32_t)caps->unbind_status);
	ffa_console_puts("[SP] FFA_NOTIFICATION_GET: ");
	print_hex32((uint32_t)caps->get_status);
	ffa_console_puts("[SP] FFA_NOTIFICATION_SET: ");
	print_hex32((uint32_t)caps->set_status);

	ffa_console_puts("[SP] NPI Feature Query: ");
	print_hex32((uint32_t)caps->npi_status);
	if (caps->npi_status == 0) {
		ffa_console_puts("	NPI Interrupt ID: ");
		print_hex32(caps->npi_intid);
	}

	ffa_console_puts("[SP] SRI Feature Query: ");
	print_hex32((uint32_t)caps->sri_status);
	if (caps->sri_status == 0) {
		ffa_console_puts("	SRI Interrupt ID: ");
		print_hex32(caps->sri_intid);
	}

	ffa_console_puts("[SP] Notification Properties Query: ");
	print_hex32((uint32_t)caps->notification_properties_status);
	if (caps->notification_properties_status == 0) {
		ffa_console_puts("[SP] Notification Properties: ");
		print_hex32(caps->notification_properties);

		ffa_console_puts(caps->vm_per_vcpu_notifications_supported ?
				"[SP] VM per-vCPU Notifications: supported\r\n" :
				"[SP] VM per-vCPU Notifications: not supported\r\n");

		ffa_console_puts(caps->sp_per_vcpu_notifications_supported ?
				"[SP] SP per-vCPU Notifications: supported\r\n" :
				"[SP] SP per-vCPU Notifications: not supported\r\n");

		ffa_console_puts("[SP] VM application notification count: ");
		print_hex32(caps->vm_notification_count);
		ffa_console_puts("[SP] SP application notification count: ");
		print_hex32(caps->sp_notification_count);
	}
}

static void prepare_msg_wait(struct ffa_regs *regs)
{
	unsigned int i;

	for (i = 0; i < FFA_REG_COUNT; i++)
		regs->x[i] = 0;

	regs->x[0] = FFA_FID_MSG_WAIT;
}

static void wait_forever(void)
{
	for (;;) {
		asm volatile("wfe");
	}
}

/* Check the incoming request and generate a response for `Direct Messaging`
 *
 * Register arrangments as per ABI:
 *
 * Request:
 *
 * w0 = FFA_MSG_SEND_DIRECT_REQ_32
 * w1 = IDs combination of sender and receiver [sender_id << 16 | receiver_id]
 * w2 = flags
 * w3 - w7 = application request payload [a small application level protocol defined for testing
 *	a service that SP provides of XORing a input request value, more detail in note]
 *
 * Response:
 *
 * w0 = FFA_MSG_SEND_DIRECT_RESP_32
 * w1 = IDs combination of sender and receiver [responder_id << 16 | sender_id]
 * w2 = flags
 * w3 - w7 = response payload */
static void prepare_direct_response(struct ffa_regs *regs,
		uint16_t my_id)
{
	ffa_console_puts("[DEBUG]: process_direct_response() called\r\n");

	uint32_t packed_ids;
	uint16_t sender_id, receiver_id;
	uint32_t flags, magic, opcode, nonce, input, reserved;
	uint32_t status = SP_STATUS_OK;

	packed_ids = (uint32_t)regs->x[1];

	sender_id = (uint16_t)(packed_ids >> 16);
	receiver_id = (uint16_t)(packed_ids & 0xffff);

	flags = (uint32_t)regs->x[2];
	magic = (uint32_t)regs->x[3];
	opcode = (uint32_t)regs->x[4];
	nonce = (uint32_t)regs->x[5];
	input = (uint32_t)regs->x[6];
	reserved = (uint32_t)regs->x[7];

	// validate the request
	if (sender_id != LINUX_ENDPOINT_ID)
		status = SP_STATUS_BAD_SENDER;
	else if (receiver_id != my_id)
		status = SP_STATUS_BAD_RECEIVER;
	else if (flags != 0)
		status = SP_STATUS_BAD_FLAGS;
	else if (magic != REQUEST_MAGIC)
		status = SP_STATUS_BAD_MAGIC;
	else if (opcode != OPCODE_XOR)
		status = SP_STATUS_BAD_OPCODE;
	else if (reserved != 0)
		status = SP_STATUS_BAD_RESERVED;

	
	regs->x[0] = FFA_FID_MSG_SEND_DIRECT_RESP_32;
	
	regs->x[1] = ((uint32_t)my_id << 16) | (uint32_t)sender_id;
	regs->x[2] = 0;
	regs->x[3] = RESPONSE_MAGIC;
	regs->x[4] = status;
	regs->x[5] = nonce;
	regs->x[6] = input ^ XOR_MASK;
	regs->x[7] = RESPONSE_TRAILER;
}

/* Check the incoming request and generate a response for `Direct Messaging v2`
 *
 * Register arrangments as per ABI:
 *
 * Request:
 *
 * x0 = FFA_MSG_SEND_DIRECT_REQ2_64
 * x1 = IDs combination of sender and receiver [sender_id << 16 | receiver_id]
 * x2 = Destination partition UUID - lower 64 bits
 * x3 = Destination partition UUID - higher 64 bits
 * x4 - x17 = application request payload - 14 registers [a small application level protocol defined for testing
 *	a service that SP provides of XORing a input request value, more detail in note]
 *
 * Response:
 *
 * x0 = FFA_MSG_SEND_DIRECT_RESP2_64
 * x1 = IDs combination of sender and receiver [responder_id << 16 | sender_id]
 * x2,x3 = reserved
 * x4 - x17 = response payload */
static void prepare_direct2_response(struct ffa_regs *regs,
		uint16_t my_id)
{
	ffa_console_puts("[DEBUG]: process_direct2_response() called\r\n");

	uint32_t packed_ids;
	uint16_t sender_id, receiver_id;
	uint64_t uuid_lo, uuid_hi;
	uint32_t magic, opcode, seq_no;
	uint32_t status = SP_STATUS_OK;

	packed_ids = (uint32_t)regs->x[1];
	sender_id = (uint16_t)(packed_ids >> 16);
	receiver_id = (uint16_t)(packed_ids & 0xffff);

	uuid_lo = regs->x[2];
	uuid_hi = regs->x[3];
	magic = regs->x[4];
	opcode = regs->x[5];
	seq_no = regs->x[6];

	if (uuid_lo != SP_UUID_LO || uuid_hi != SP_UUID_HI) {
		ffa_console_puts("[SP] Destination Partition UUID as advertised in the request: \r\n");
		print_hex64(uuid_hi);
		print_hex64(uuid_lo);
		status = SP_STATUS_WRONG_UUID;
	}

	if (sender_id != LINUX_ENDPOINT_ID)
		status = SP_STATUS_BAD_SENDER;
	else if (receiver_id != my_id)
		status = SP_STATUS_BAD_RECEIVER;
	else if (magic != REQUEST_MAGIC_v2)
		status = SP_STATUS_BAD_MAGIC;
	else if (opcode != OPCODE_XOR)
		status = SP_STATUS_BAD_OPCODE;

	regs->x[0] = FFA_FID_MSG_SEND_DIRECT_RESP2_64;

	uint32_t response_ids = ((uint32_t)my_id << 16) | (uint32_t)sender_id;
	regs->x[1] = response_ids;
	regs->x[2] = 0;
	regs->x[3] = 0;
	regs->x[4] = RESPONSE_MAGIC_v2;
	regs->x[5] = status;
	regs->x[6] = seq_no;

	for (int i = 0; i < 11; i++) {
		uint64_t input, output;
		input = (uint64_t)regs->x[i+7];
		output = input ^ XOR_MASK_v2;

		regs->x[i + 7] = output;
	}

	(void)uuid_lo;
	(void)uuid_hi;
}

/* application payload protocol for Indirect Messaging */
#define SP_RX_BUFFER_ADDR	0x07700000ULL
#define SP_TX_BUFFER_ADDR	0x07701000ULL

// TODO: this we get from the features query so use that value instead of static variable
#define SP_RX_TX_BUFFER_SIZE	4096U

#define INDIRECT_REQUEST_MAGIC \
				0x494e4431U	/* IND1 */
#define INDIRECT_RESPONSE_MAGIC	\
				0x494e4432U	/* IND2 */
#define INDIRECT_OPCODE_XOR     1U
#define INDIRECT_STATUS_OK      0U

#define INDIRECT_XOR_MASK \
			UINT64_C(0xa5a5a5a5a5a5a5a5)

/* fixed-size test payload */
struct indirect_test_msg {
	uint32_t magic;
	uint32_t opcode;
	uint32_t sequence;
	uint32_t count;

	uint64_t values[4];
};

_Static_assert(sizeof(struct indirect_test_msg) == 48,
		"Unexpected indirect request payload size");

static bool uuid_matches(const uint8_t uuid[16])
{
	for (unsigned int i = 0; i < 16; i++) {
		if (uuid[i] != SP_UUID_BYTES[i])
			return false;
	}
	return true;
}

static void process_indirect_message(uint16_t my_id)
{
	ffa_console_puts("[DEBUG]: process_indirect_message() called\r\n");

	struct ffa_indirect_msg_header *header;
	struct indirect_test_msg request, response;

	void *rx_buffer = (void *)(uintptr_t)SP_RX_BUFFER_ADDR;
	void *tx_buffer = (void *)(uintptr_t)SP_TX_BUFFER_ADDR;
	uint32_t hyp_notifications;
	uint32_t sender_id, receiver_id, offset, size;
	bool msg_valid;
	int ret;

	ret = ffa_notification_get_hyp_framework(my_id, 0, &hyp_notifications);
	if (ret) {
		ffa_console_puts("[SP]: FFA_NOTIFICATION_GET failed\r\n");
		return;
	}

	if (!(hyp_notifications & FFA_NOTIFICATION_RX_BUFFER_FULL))
		return;

	// having this notification means the SPMC has put the indirect message into the Rx Buffer
	asm volatile("dmb ish" ::: "memory");

	header = rx_buffer;

	offset = header->offset;
	size = header->size;

	sender_id = header->sender_receiver_id >> 16;
	receiver_id = header->sender_receiver_id & 0xffff;

	msg_valid = 
		header->flags == 0 &&
		header->reserved0 == 0 &&
		header->reserved1 == 0 &&
		sender_id == LINUX_ENDPOINT_ID &&
		receiver_id == my_id &&
		offset >= sizeof(*header) &&
		offset <= SP_RX_TX_BUFFER_SIZE &&
		size == sizeof(request) &&
		size <= SP_RX_TX_BUFFER_SIZE - offset &&
		uuid_matches(header->uuid);

	// consume the message
	if (msg_valid) {
		const uint8_t *src = (const uint8_t *)rx_buffer + offset;

		uint8_t *dst = (uint8_t *)&request;

		for (unsigned int i = 0; i < sizeof(request); i++)
			dst[i] = src[i];
	}

	// release the Rx Buffer after consuming the message
	ret = ffa_rx_release();

	if (ret) {
		ffa_console_puts("[SP]: FFA_RX_RELEASE failed\r\n");
		return;
	}

	if (!msg_valid) {
		ffa_console_puts("[SP]: Invalid Indirect Message Header and request\r\n");
		return;
	}

	if (request.magic != INDIRECT_REQUEST_MAGIC || 
			request.opcode != INDIRECT_OPCODE_XOR ||
			request.count != 4) {
		ffa_console_puts("[SP] Invalid indirect application message\r\n");
		return;
	}

	ffa_console_puts("[SP]: Indirect request received and validated\r\n");

	//prepare the response now
	response.magic = INDIRECT_RESPONSE_MAGIC;
	response.opcode = INDIRECT_STATUS_OK;
	response.sequence = request.sequence;
	response.count = request.count;

	for (unsigned int i = 0; i < 4; i++) {
		response.values[i] = request.values[i] ^ INDIRECT_XOR_MASK;
	}

	// send the indirect response to Linux endpoint
	ret = ffa_msg_send2(tx_buffer,
			SP_RX_TX_BUFFER_SIZE,
			my_id,
			LINUX_ENDPOINT_ID,
			SP_UUID_BYTES,
			&response,
			sizeof(response));

	if (ret == 0)
		ffa_console_puts("[SP] Indirect response send succeeded\r\n");
	else
		ffa_console_puts("[SP] Indirect response send failed\r\n");

	if (ret) {
		ffa_console_puts("[SP]: FFA_MSG_SEND2 response sending failed\r\n");
		return;
	}

	ffa_console_puts("[SP]: Indirect Response sent to Linux\r\n");
}

void sp_main(void)
{
	uint16_t my_id = 0;
	struct ffa_regs regs = {0};
	struct ffa_notifications_capabilities notification_caps = {0};

	ffa_console_puts("[SP] : [DEBUG]: sp_main() called\r\n");

	// NOTE: FF-A v1.2+ should be requested because minimum requirement for Virtio-msg later
	int ret = ffa_init_from_manifest(FFA_VERSION_1_3);
	//int ret = ffa_init(FFA_VERSION_1_3);

	if (ret != 0)
		wait_forever();

	if (!ffa_console_log_available())
		wait_forever();

	ffa_console_puts(
			"\r\n"
			"============================\r\n"
			"	FFA LAB SP BOOTED    \r\n"
			"============================\r\n"
			"\r\n"
	);

	ffa_console_puts_32("\r\n[SP] logged via 32 bit interface\r\n");

	ffa_console_puts("[SP] FF-A version: 1.3 requested\r\n");
	
	if (ffa_get_id(&my_id) != 0) {
		ffa_console_puts("[SP] FFA_ID_GET failed\n");
		wait_forever();
	}

	ffa_console_puts("[SP] Endpoint ID: ");
	print_hex16(my_id);

	if (my_id != SP_ENDPOINT_ID) {
		ffa_console_puts("[SP] Unexpected Endpoint ID\n");
		wait_forever();
	}

	ret = ffa_notification_capabilities_get(&notification_caps);
	if (ret) {
		ffa_console_puts("[SP]: Notification Capability Discovery Failed\r\n");
		print_hex32((uint32_t)ret);
		wait_forever();
	}
	report_notifications_features(&notification_caps);

	if (notification_caps.npi_status != 0 ||
		notification_caps.npi_intid == HF_INVALID_INTID) {
		ffa_console_puts("[SP] NPI Feature unavailable, cannot receive notifications\r\n");
		wait_forever();
	}
	sp_npi_intid = notification_caps.npi_intid;
	
	ret = ffa_notification_bind(LINUX_ENDPOINT_ID, my_id, false, LINUX_TO_SP_NOTIF_BIT);
	if (ret) {
		ffa_console_puts("[SP] FFA_NOTIFICATION_BIND failed \r\n");
		print_hex32((uint32_t)ret);
		wait_forever();
	}
	ffa_console_puts("[SP] Bound Linux as sender for global Notification ID: 19\r\n");

	report_all_ffa_functions_check(functions_to_check,
			ARRAY_SIZE(functions_to_check));

	uint32_t min_size, max_size;

	ret = ffa_rxtx_get_limits(&min_size, &max_size);
	if (ret) {
		ffa_console_puts("[SP]: RXTX_MAP feature query failed\n");
		wait_forever();
	}

	ffa_console_puts("[SP]: Rx/Tx Minimum Size: ");
	print_hex32(min_size);
	ffa_console_puts("[SP]: Rx/Tx Maximum Size: ");
	print_hex32(max_size);
	
	// check the min_size, max_size of pages, and rx/tx buffer memory addresses PA or IPA to be page aligned- as it FFA requirement
	if (min_size > FFA_RXTX_PAGE_SIZE ||
		(max_size && max_size < FFA_RXTX_PAGE_SIZE) ||
		(0x07700000ULL % min_size) || (0x07701000ULL % min_size)) {
		
		ffa_console_puts("[SP]: Manifest Rx/Tx Buffers violate FF-A limits\r\n");
		wait_forever();
	}

	ffa_console_puts(
			"[SP] Initialization complete \r\n"
			"[SP] Calling FFA_MSG_WAIT\r\n"
	);

	/*
	 * Startup initialized the SP with all DAIF exceptions masked.
	 * The IRQ vector is now installed, so allow virtual IRQ delivery.
	 *
	 * DAIFCLR bit 1 corresponds to the IRQ mask (I).
	 * Keep D, A and F masked for now.
	*/

	asm volatile(
			"msr daifclr, #2\n\t"
			"isb\n\t"
			::: "memory"
			);

	/* FFA_MSG_WAIT FFA call from SP allows Hafnium to resume its scheduling to 
	 * schedule other tasks or return control to Normal World
	 * * if MSG_WAIT returns, meaning the same SP has been resumed by SPMC 
	 * */
	prepare_msg_wait(&regs);

	for (;;) {

		ffa_smc_call(&regs);

		switch((uint32_t)regs.x[0]) {
			case FFA_FID_MSG_SEND_DIRECT_REQ_32:
				ffa_console_puts("[SP] Direct Request Message Received\n");
				prepare_direct_response(&regs, my_id);
				continue;

			case FFA_FID_MSG_SEND_DIRECT_REQ2_64:
				ffa_console_puts("[SP] Direct Request v2 Message Received\n");
				prepare_direct2_response(&regs, my_id);
				continue;

			case FFA_FID_RUN:
				ffa_console_puts("[SP]: FFA_RUN received\r\n");
				process_indirect_message(my_id);
				prepare_msg_wait(&regs);
				continue;

			case FFA_FID_SUCCESS:
				ffa_console_puts("[SP]: FFA_SUCCESS return\r\n");
				prepare_msg_wait(&regs);
				continue;

			case FFA_FID_SUCCESS_64:
				ffa_console_puts("[SP]: FFA_SUCCESS_64 return\r\n");
				prepare_msg_wait(&regs);
				continue;

			case FFA_FID_ERROR:
				prepare_msg_wait(&regs);
				continue;

			case FFA_FID_INTERRUPT:
				/* FFA_INTERRUPT is a SPMC-to-SP signal delivered through ERET mechanism
				 * and is not a normal FF-A function call
				 *
				 * especially when SPMC wakes an SP up that is waiting or is blocked
				 *
				 * if SPMC signaled a pending interrupt through FF-A return path, 
				 * the architectural vIRQ handler, the exception handler services and
				 * acknowledges it
				 * */

				ffa_console_puts("[SP]: FFA_INTERRUPT received\r\n");
				break;

			default:
				ffa_console_puts("[SP] Unexpected FF-A return\n");
				print_hex32((uint32_t)regs.x[0]);
				prepare_msg_wait(&regs);	
				continue;
		}
	}
}
