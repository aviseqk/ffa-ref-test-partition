### Issue with using absolute pointers to string literals in SP code, creating relocation problems with our PIE SP

BUG: REPORT: I tried using const char *name here for function name, as string literal,
 * but Hafnium was reporting stage-2 fault errors whilst doing that.
 * Reason: Because then those were pointers to string literals,
 * those pointers needed dynamic relocation, if not applied, then the linker
 * was retaining its link-time value, e.g. 0x2a18, instead of the runtime
 * value like 0x70002a18, and thus when Hafnium tried to read that linktime address
 * 0x2a18, it's stage-2 translation was rejecting the address with following error
 * Now, our SP's ELF is a `ET_DYN` image but a mechanism should have to apply relocation 
 * dynamic relocations at runtime, that the pointers might have needed, but for now I resolved
 * it by simply using characters in the string instead of storing pointers
 *
 * WARNING: Stage-2 page fault: pc=0x700214c, vmid=0x8001, vcpu=0, vaddr=0x2a18, ipaddr=0x2a18, mode=0x1 0x7c
 * NOTICE: Injecting Data Abort exception into VM 0x8001.
 *
 *
 * Another Resolve: For linker emitting relocation table, and the relocation being applied at runtime by Hafnium while loading
 * ISSUE: my linker script did not have the facility to emit the relocations required by the string pointer table,
 * it was not creating any relocation table, i added the .data and .data.rel.ro section, and now the relocation table was being generated
 * as R_AARCH64_RELATIVE entries in the image
 *
```sh
 ffa-ref-test-partition on  main ❯  aarch64-none-linux-gnu-readelf -rW build/ffa-ref-
 test-sp.elf

 Relocation section '.rela.dyn' at offset 0x2d88 contains 13 entries:
    Offset             Info             Type               Symbol's Value  Symbol's Name + Addend
    0000000000002800  0000000000000403 R_AARCH64_RELATIVE                        2be8
    0000000000002810  0000000000000403 R_AARCH64_RELATIVE                        2c00
    0000000000002820  0000000000000403 R_AARCH64_RELATIVE                        2c18
    0000000000002830  0000000000000403 R_AARCH64_RELATIVE                        2c28
    0000000000002840  0000000000000403 R_AARCH64_RELATIVE                        2c40
    0000000000002850  0000000000000403 R_AARCH64_RELATIVE                        2c50
    0000000000002860  0000000000000403 R_AARCH64_RELATIVE                        2c60
    0000000000002870  0000000000000403 R_AARCH64_RELATIVE                        2c70
    0000000000002880  0000000000000403 R_AARCH64_RELATIVE                        2c80
    0000000000002890  0000000000000403 R_AARCH64_RELATIVE                        2c90
    00000000000028a0  0000000000000403 R_AARCH64_RELATIVE                        2ca8
    00000000000028b0  0000000000000403 R_AARCH64_RELATIVE                        2cc0
    00000000000028c0  0000000000000403 R_AARCH64_RELATIVE                        2cd8
    ```
 *
 * SO, now the Linker is emitting the relocations required, but those relocations need to be applied still, that's separate.
 * as linker flags have the -fpie and -pie, hence linker is producing a PIE with dynamic relocation metadata
 * BUT, the issue was still that this is metadata, I have to make sure the loading mechanism performs this relocations.
 *
 * ISSUE: so, if Hafnium copies this image into its memory without processing the .rela.dyn section, the relocations are never being performed.
 * also, I am converting the raw ELF into .bin using objcopy -o binary, and apparently, .rela.dyn section is not carried into raw binary as relocation instructions that way.
 * 
 * The resulting binary contains the loadable section contents, laid out according to their load addresses. 
 * It does not retain the ELF program headers, dynamic section metadata, or relocation records 
 * as ELF metadata.
 * Consequently, Hafnium receives an image without the relocation instructions that my linker generated.
 * 
 * TODO: I could either, make the SP self-relocating, as in add code in my SP to process it during its startup,
 * where it relocates its R_AARCH64_RELATIVE entries with load base address,
 * OR, I could write a relocation-free SP code, as in instead of using absolute pointers, i could use relative-offsets
 * within members, or position-independent address calculations in the code instead, 
 * BUT THESE ARE ADVANCED PARTS OF THE PROGRAM COMPILING, LOADING and PACKAGING, so TODO later.
 *
 * FOR NOW, I just replaced the const char *name with const char name[32], 
 * totally avoiding the use of absolute pointers i.e. to the string literals and using character arrays instead`
