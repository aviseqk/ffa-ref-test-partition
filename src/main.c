#include "ffa/ffa_abi.h"
#include <stdint.h>

/* iur SP endpoint ID */
#define SP_ENDPOINT_ID       0x8001

/* Linux normal-world endpoint ID. */
#define LINUX_ENDPOINT_ID    0x0000

/* Application protocol. */
#define REQUEST_MAGIC        UINT32_C(0x4c4e5831) /* LNX1 */
#define RESPONSE_MAGIC       UINT32_C(0x53505231) /* SPR1 */

#define OPCODE_XOR		UINT32_C(1)
#define REQUEST_NONCE        UINT32_C(0x1234abcd)
#define REQUEST_VALUE        UINT32_C(0x11223344)
#define XOR_MASK             UINT32_C(0xa5a5a5a5)
#define RESPONSE_TRAILER     UINT32_C(0x53504f4e) /* SPON */

/* Protocol response status codes. */

#define SP_STATUS_OK             0
#define SP_STATUS_BAD_SENDER     1
#define SP_STATUS_BAD_RECEIVER   2
#define SP_STATUS_BAD_FLAGS      3
#define SP_STATUS_BAD_MAGIC      4
#define SP_STATUS_BAD_OPCODE     5
#define SP_STATUS_BAD_RESERVED   6

static void prepare_msg_wait(struct ffa_regs *regs)
{
	unsigned int i;

	for (i = 0; i < 8; i++)
		regs->x[i] = 0;

	regs->x[0] = FFA_MSG_WAIT_32;
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

/* Check the incoming request and generate a response 
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

	
	regs->x[0] = FFA_MSG_SEND_DIRECT_RESP_32;
	
	regs->x[1] = ((uint32_t)my_id << 16) | (uint32_t)sender_id;
	regs->x[2] = 0;
	regs->x[3] = RESPONSE_MAGIC;
	regs->x[4] = status;
	regs->x[5] = nonce;
	regs->x[6] = input ^ XOR_MASK;
	regs->x[7] = RESPONSE_TRAILER;
}

void sp_main(void)
{
	if (!ffa_console_log_available())
		wait_forever();

	ffa_console_puts(
			"\r\n"
			"============================\r\n"
			"	FFA LAB SP BOOTED    \r\n"
			"============================\r\n"
			"\r\n"
	);

	ffa_console_puts("[SP] Executing C code at S-EL1\r\n");
	
	uint16_t my_id = 0;
	struct ffa_regs regs = {0};

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
	
	ffa_console_puts(
			"[SP] Initialization complete \r\n"
			"[SP] Calling FFA_MSG_WAIT\r\n"
	);

	/* FFA_MSG_WAIT_32 FFA call from SP allows Hafnium to resume its scheduling to 
	 * schedule other tasks or return control to Normal World
	 * * if MSG_WAIT returns, meaning the same SP has been resumed by SPMC 
	 * */
	prepare_msg_wait(&regs);

	for (;;) {

		ffa_smc_call(&regs);

		switch((uint32_t)regs.x[0]) {
			case FFA_MSG_SEND_DIRECT_REQ_32:
				ffa_console_puts("[SP] Direct Request Message Received\n");
				prepare_direct_response(&regs, my_id);
				continue;

			case FFA_SUCCESS_32:
				prepare_msg_wait(&regs);
				continue;

			case FFA_ERROR_32:
				prepare_msg_wait(&regs);
				continue;

			default:
				ffa_console_puts("[SP] Unexpected FF-A return\n");
				prepare_msg_wait(&regs);	
				continue;
		}
	}

}
