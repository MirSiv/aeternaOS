OUTPUT_FORMAT("elf64-x86-64")
ENTRY(_start)

SECTIONS {
    . = 0x100000;

    kernel_header_start = .;

    .text : ALIGN(8) {
        KEEP(*(.multiboot2))
        *(.text)
        *(.text.*)
    }

    .rodata : ALIGN(4K) {
        *(.rodata)
        *(.rodata.*)
    }

    .data : ALIGN(4K) {
        *(.data)
        *(.data.*)
    }

    kernel_code_end = .;

    .bss : ALIGN(4K) {
        sbss = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        ebss = .;
    }

    kernel_header_end = .;

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note.gnu.property)
    }
}