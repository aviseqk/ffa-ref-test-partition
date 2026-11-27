### Developer Note 
As the Arm's FVP has four separate PL011 UART instances, and as it were, UART0 was being used by TF-A, then passed onto U-Boot
and subsequently Linux for boot messages and so, but midway since Hafnium was also being loaded as SPMC but in debug mode, 
all the SPMC diagnostics, the boot verbose messages, and then the SP's FFA console messages were also passing onto the UART0 as well.

This was convoluting the log messages on a single console i.e. the UART0. We also had the SPMC trace calls being logged onto the UART1 
but still the SPMC diagnostics and the SP's FFA_CONSOLE messages were still being directed to UART0 i.e. `0x1c090000`

This was because, Hafnium when built, its platform `secure_aem_v8a_fvp_vhe` had pl011_base_address as `0x1c090000` meaning it was using
UART0 for SPMC. 
in the reference file `project/reference/BUILD.gn`, under the `aarch64_toolchains("secure_aem_v8a_fvp_vhe") {` target

Solution:
	For I wanted to use UART3 of this setup for the separate Hafnium SPMC diagnostics and Secure Parittion messages emitted via
	the `FFA_CONSOLE` interface, so I changed hafnium's console output to the `pl011_base_address=0x1c0c0000` i.e. UART3

	TODO: Make this a script for patch change so that the patch to hafnium applies only when we need SP msgs in a separate console
