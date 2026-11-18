#include "ffa/ffa_abi.h"
#include <stdint.h>

/* iur SP endpoint ID */
#define SP_ENDPOINT_ID		0x8001

/* Linux normal-world endpoint ID. */
#define LINUX_ENDPOINT_ID	0x0000

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

#define SP_STATUS_OK             0
#define SP_STATUS_BAD_SENDER     1
#define SP_STATUS_BAD_RECEIVER   2
#define SP_STATUS_BAD_FLAGS      3
#define SP_STATUS_BAD_MAGIC      4
#define SP_STATUS_BAD_OPCODE     5
#define SP_STATUS_BAD_RESERVED   6


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
	{"FFA_RXTX_MAP_64", FFA_FID_RXTX_MAP_64},
	{"FFA_MSG_SEND2", FFA_FID_MSG_SEND2},
	{"FFA_NOTIFICATION_BIND", FFA_FID_NOTIFICATION_BIND},
	{"FFA_NOTIFICATION_SET", FFA_FID_NOTIFICATION_SET},
	{"FFA_NOTIFICATION_GET", FFA_FID_NOTIFICATION_GET},
	//{"FFA_MEM_RETRIEVE_REQ2", FFA_FID_MEM_RETRIEVE_REQ_64},
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

static void print_hex16(uint16_t value)
{
	static const char hex[] = "0123456789abcdef";

	char buffer[8];

	buffer[0] = '0';
	buffer[1] = 'x';

	for (unsigned int i = 0; i < 4; i++) {
		const unsigned int shift = (3 - i) * 4;
		buffer[2 + i] = hex[(value >> shift) & 0xf];
	}

	buffer[6] = '\r';
	buffer[7] = '\n';

	char output[9];

	for (unsigned int i = 0; i < 8; i++) {
		output[i] = buffer[i];
	}


	output[8] = '\0';

	(void)ffa_console_puts(output);
}

/*
static void print_hex32(uint32_t value)
{
	static const char hex[] = "0123456789abcdef";

	char buffer[12];

	buffer[0] = '0';
	buffer[1] = 'x';

	for (unsigned int i = 0; i < 8; i++) {
		const unsigned int shift = (7 - i) * 4;
		buffer[2 + i] = hex[(value >> shift) & 0xf];
	}

	buffer[10] = '\r';
	buffer[11] = '\n';

	char output[13];

	for (unsigned int i = 0; i < 12; i++) {
		output[i] = buffer[i];
	}


	output[12] = '\0';

	(void)ffa_console_puts(output);
}
*/

static void print_hex64(uint64_t value)
{
	static const char hex[] = "0123456789abcdef";

	char output[22];

	output[0] = '0';
	output[1] = 'x';

	for (unsigned int i = 0; i < 16; i++) {
		const unsigned int shift = (15 - i) * 4;
		output[2 + i] = hex[(value >> shift) & 0xf];
	}

	output[18] = '\r';
	output[19] = '\n';
	output[20] = '\0';

	(void)ffa_console_puts(output);
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

	ffa_console_puts("[SP] Destination Partition UUID in the request payload: \r\n");
	print_hex64(uuid_hi);
	print_hex64(uuid_lo);

	if (sender_id != LINUX_ENDPOINT_ID)
		status = SP_STATUS_BAD_SENDER;
	else if (receiver_id != my_id)
		status = SP_STATUS_BAD_RECEIVER;
	else if (magic != REQUEST_MAGIC_v2)
		status = SP_STATUS_BAD_MAGIC;
	else if (opcode != OPCODE_XOR)
		status = SP_STATUS_BAD_OPCODE;

	regs->x[0] = FFA_FID_MSG_SEND_DIRECT_RESP2_64;

	regs->x[1] = (uint32_t)(my_id << 16) | (uint32_t)sender_id;
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

void sp_main(void)
{
	uint16_t my_id = 0;
	struct ffa_regs regs = {0};

	// NOTE: FF-A v1.2 requested because minimum requirement for Virtio-msg later
	int ret = ffa_init(FFA_VERSION_1_2);

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

	ffa_console_puts("[SP] FF-A version: 1.2 requested\r\n");
	
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
	
	report_all_ffa_functions_check(functions_to_check,
			ARRAY_SIZE(functions_to_check));

	ffa_console_puts(
			"[SP] Initialization complete \r\n"
			"[SP] Calling FFA_MSG_WAIT\r\n"
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

			case FFA_FID_SUCCESS:
				prepare_msg_wait(&regs);
				continue;

			case FFA_FID_ERROR:
				prepare_msg_wait(&regs);
				continue;

			default:
				ffa_console_puts("[SP] Unexpected FF-A return\n");
				prepare_msg_wait(&regs);	
				continue;
		}
	}
}
