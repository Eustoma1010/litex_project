/* Linker script cho LiteX RISC-V SoC
   Bộ nhớ được chia thành 3 vùng:
   - FLASH (ROM): 0x00000000 - 0x00001FFF (8 KB) - Code
   - SRAM:        0x10000000 - 0x10003FFF (16 KB) - Fast RAM
   - MAIN_RAM:    0x20000000 - 0x2003FFFF (256 KB) - Main RAM
*/

/* Bao gồm bản đồ bộ nhớ (MEMORY) tự động sinh bởi LiteX */
INCLUDE build/sim/software/include/generated/regions.ld

SECTIONS {
    /* ===== Text (Code) ===== */
    .text : {
        KEEP(*(.text.start))           /* Entry point */
        *(.text)                        /* Regular code */
        *(.text.*)                      /* Code sections */
        *(.rodata)                      /* Const data */
        *(.rodata.*)
        . = ALIGN(4);
        _etext = .;
    } > sram

    /* ===== Initialized Data ===== */
    .data : {
        _data = .;
        *(.data)                        /* Initialized data */
        *(.data.*)
        . = ALIGN(4);
        _edata = .;
    } > sram
    
    _data_load = LOADADDR(.data);       /* LMA of data section */

    /* ===== Uninitialized Data (BSS) ===== */
    .bss : {
        _bss = .;
        *(.bss)                         /* Uninitialized data */
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > sram

    /* ===== Stack ===== */
    .stack : {
        _stack_bottom = .;
        . += 0x1000;                    /* 4 KB stack */
        _stack_top = .;
    } > sram

    /* Discard unused sections */
    /DISCARD/ : {
        *(.note.GNU-stack)
        *(.gnu_debuglink)
        *(.gnu_debugdata)
    }
}

/* Export symbols cho crt0.s */
__data_start__ = ADDR(.data);
__data_end__ = ADDR(.data) + SIZEOF(.data);
__data_size__ = SIZEOF(.data);
