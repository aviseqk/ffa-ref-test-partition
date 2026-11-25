   ```C
   case FFA_FID_INTERRUPT:
				uint32_t signaled_intid = (uint32_t)regs.x[2];
				uint32_t handled_intid = sp_last_irq_id;

				ffa_console_puts("[SP]: FFA_INTERRUPT received\r\n");
				ffa_console_puts("[SP] Interrupt ID from FFA_INTERRUPT:\r\n");
				print_hex32(signaled_intid);

				ffa_console_puts("[SP] Interrupt ID from HF_INTERRUPT_GET:\r\n");
				print_hex32(handled_intid);

				if (signaled_intid != handled_intid) {
					ffa_console_puts("[SP]: ERROR: IRQ was not acknowledged correctly\r\n");

					// stop immediately rather than continuing into the MSG_WAIT and creating a infinite interrupt loop
					asm volatile("msr daifset, #2" ::: "memory");
					wait_forever();
				}

				if (signaled_intid == HF_NOTIFICATION_PENDING_INTID) {
					ffa_console_puts("[SP] Notification Pending Interrupt (NPI) handled\r\n");
				} else {
					ffa_console_puts("[SP] Other Virtual Interrupt handled\r\n");
				}

				sp_last_irq_id = HF_INVALID_INTID;
				
				// interrupt handling is complete, SP can go back to its WAITING state flow
	
				prepare_msg_wait(&regs);
				continue;

   ```
