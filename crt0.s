/* crt0.s - RISC-V Startup Code (Assembly)
   Khởi tạo hệ thống trước khi gọi main()
   Kiến trúc: RISC-V RV32I
*/

.section .text.start, "ax"
.global _start

_start:
    /* ===== Setup Stack Pointer ===== */
    la  sp, _stack_top          /* Load stack pointer từ linker script */

    /* ===== Clear BSS Section ===== */
    la  t0, _bss                /* t0 = địa chỉ BSS bắt đầu */
    la  t1, _ebss               /* t1 = địa chỉ BSS kết thúc */
    
clear_bss:
    beq t0, t1, clear_bss_done  /* Nếu t0 == t1, kết thúc */
    sw  zero, 0(t0)             /* Ghi 0 vào *t0 */
    addi t0, t0, 4              /* t0 += 4 */
    jal zero, clear_bss         /* Lặp lại */

clear_bss_done:

    /* ===== Initialize Data Section (copy from FLASH to DRAM) ===== */
    la  t0, _data_load          /* t0 = LMA (FLASH) */
    la  t1, _data               /* t1 = VMA (DRAM) bắt đầu */
    la  t2, _edata              /* t2 = VMA (DRAM) kết thúc */

copy_data:
    beq t1, t2, copy_data_done  /* Nếu bằng nhau, kết thúc */
    lw  t3, 0(t0)               /* Đọc từ FLASH */
    sw  t3, 0(t1)               /* Ghi vào DRAM */
    addi t0, t0, 4              /* Tăng LMA */
    addi t1, t1, 4              /* Tăng VMA */
    jal zero, copy_data         /* Lặp lại */

copy_data_done:

    /* ===== Setup Global Pointer (GP) - Optional ===== */
    /* Bỏ qua nếu không cần gp-relative addressing */
    /* la  gp, __global_pointer$ */

    /* ===== Call main() ===== */
    jal zero, main              /* Jump to main (không cần return) */

    /* ===== Infinite Loop (nếu main return) ===== */
    jal zero, _start            /* Restart nếu main kết thúc */

/* Export symbols */
.global _stack_top
.global _stack_bottom
.global _bss
.global _ebss
.global _data
.global _edata
.global _data_load
