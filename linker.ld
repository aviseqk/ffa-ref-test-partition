OUTPUT_ARCH(aarch64)

ENTRY(_start)

SECTIONS
{
	/* link-time image origin */
	. = 0x1000;

	.text :
	{
		KEEP(*(.text.boot))
		*(.text)
		*(.text.*)
	}

	. = ALIGN(2048);

	.vectors :
	{
		KEEP(*(.vectors))
	}

	. = ALIGN(16);

	.rodata : 
	{
		*(.rodata)
		*(.rodata.*)
	}

	. = ALIGN(16);

	.bss (NOLOAD) :
	{
		__bss_start = .;

		*(.bss)
		*(.bss.*)
		*(COMMON)

		. = ALIGN(8);

		__bss_end = .;
	}

	. = ALIGN(16);

	/* reserving 16KiB for stack, primary execution context */
	.stack (NOLOAD) :
	{
		__stack_bottom = .;

		. += 0x4000;

		__stack_top = .;
	}

	/DISCARD/ :
	{
		*(.comment)
		*(.note*)
		*(.eh_frame*)
	}
}
