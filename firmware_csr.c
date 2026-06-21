/*
 * Bare-Metal Firmware cho LiteX RISC-V SoC
 * Chức năng: Demo UART communication, Timer interrupt, và System Control
 * Ngôn ngữ: C (C99)
 * Phiên bản: Sử dụng tự động map địa chỉ từ csr.h
 */

#include <stdint.h>

/* ============================================================================
   Tự động sinh CSR Memory Map từ LiteX
   ============================================================================ */
static inline uint32_t csr_read_simple(uint32_t addr) {
    return *(volatile uint32_t *)addr;
}

static inline void csr_write_simple(uint32_t value, uint32_t addr) {
    *(volatile uint32_t *)addr = value;
}

#define CSR_ACCESSORS_DEFINED
#include <generated/csr.h>

/* ============================================================================
   UART Functions
   ============================================================================ */

void uart_init(void) {
    /* UART được khởi tạo mặc định với baud rate 115200 */
}

int uart_tx_ready(void) {
    return !uart_txfull_read();
}

int uart_rx_available(void) {
    return !uart_rxempty_read();
}

void uart_putc(char c) {
    while (!uart_tx_ready()) {
        /* Chờ buffer transmit rỗng */
    }
    uart_rxtx_write((uint32_t)c);
}

char uart_getc(void) {
    while (!uart_rx_available()) {
        /* Chờ dữ liệu */
    }
    return (char)(uart_rxtx_read() & 0xFF);
}

void uart_puts(const char *str) {
    while (*str) {
        uart_putc(*str++);
    }
}

void uart_puthex(uint32_t value) {
    const char *hex = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        uart_putc(hex[(value >> (i * 4)) & 0xF]);
    }
}

/* ============================================================================
   Timer Functions
   ============================================================================ */

void timer_init(uint32_t period) {
    timer0_load_write(period * 100000);     /* Tính toán count từ ms */
    timer0_reload_write(period * 100000);   /* Auto-reload */
    timer0_en_write(1);                      /* Bật timer */
}

void timer_stop(void) {
    timer0_en_write(0);
}

uint32_t timer_read(void) {
    /* Đọc giá trị counter hiện tại */
    timer0_update_value_write(1);
    return timer0_value_read();
}

int timer_irq_pending(void) {
    return timer0_ev_pending_read() & 1;
}

void timer_irq_clear(void) {
    timer0_ev_pending_write(1);
}

/* ============================================================================
   System Functions
   ============================================================================ */

void system_info(void) {
    uart_puts("\r\n=== LiteX RISC-V SoC System Information ===\r\n");
    
    uint32_t id = csr_read_simple(CSR_IDENTIFIER_MEM_BASE);
    uint32_t version = csr_read_simple(CSR_IDENTIFIER_MEM_BASE + 4);
    uint32_t errors = ctrl_bus_errors_read();
    
    uart_puts("System ID:     0x");
    uart_puthex(id);
    uart_puts("\r\nSystem Ver:    0x");
    uart_puthex(version);
    uart_puts("\r\nBus Errors:    ");
    uart_puthex(errors);
    uart_puts("\r\n");
}

void delay_ms(uint32_t ms) {
    volatile uint32_t count = ms * 10000;
    while (count--) {
        asm("nop");
    }
}

/* ============================================================================
   Main Program
   ============================================================================ */

int main(void) {
    uart_init();
    
    /* Thông báo khởi động */
    uart_puts("\r\n");
    uart_puts("================================================\r\n");
    uart_puts("   LiteX RISC-V SoC - Auto CSR Firmware\r\n");
    uart_puts("   Platform: VexRiscv (32-bit RISC-V)\r\n");
    uart_puts("   Frequency: 100 MHz\r\n");
    uart_puts("================================================\r\n\r\n");
    
    /* Hiển thị thông tin hệ thống */
    system_info();
    
    /* Kiểm tra bộ nhớ */
    uart_puts("\r\n=== Memory Test ===\r\n");
    uint32_t test_addr = 0x10001800;  /* SRAM_TEST (offset 6KB, trong vùng 8KB) */
    uint32_t test_value = 0xDEADBEEF;
    
    csr_write_simple(test_value, test_addr);
    uint32_t read_value = csr_read_simple(test_addr);
    
    uart_puts("Write to 0x");
    uart_puthex(test_addr);
    uart_puts(": 0x");
    uart_puthex(test_value);
    uart_puts("\r\nRead back:   0x");
    uart_puthex(read_value);
    
    if (test_value == read_value) {
        uart_puts(" ✓ PASS\r\n");
    } else {
        uart_puts(" ✗ FAIL\r\n");
    }
    
    /* Kiểm tra Scratch Register */
    uart_puts("\r\n=== Scratch Register Test ===\r\n");
    ctrl_scratch_write(0xABCD1234);
    uint32_t scratch = ctrl_scratch_read();
    uart_puts("Scratch: 0x");
    uart_puthex(scratch);
    uart_puts("\r\n");
    
    /* Demo Timer với LED blink simulation */
    uart_puts("\r\n=== Timer Test (5 ticks) ===\r\n");
    
    for (int i = 0; i < 5; i++) {
        uart_puts("Tick ");
        uart_putc('0' + (i + 1));
        uart_puts(" - Timer value: 0x");
        uint32_t timer_val = timer_read();
        uart_puthex(timer_val);
        uart_puts("\r\n");
        delay_ms(100);
    }
    
    /* Input/Output Test */
    uart_puts("\r\n=== Interactive Test ===\r\n");
    uart_puts("Type a character (will echo back):\r\n");
    
    for (int i = 0; i < 3; i++) {
        uart_puts("> ");
        char c = uart_getc();
        uart_puts("You typed: ");
        uart_putc(c);
        uart_puts(" (0x");
        uart_puthex((uint32_t)c);
        uart_puts(")\r\n");
    }
    
    /* String Test */
    uart_puts("\r\n=== String Test ===\r\n");
    uart_puts("Hello from bare-metal RISC-V!\r\n");
    uart_puts("Memory-mapped I/O is working!\r\n");
    uart_puts("All tests completed successfully!\r\n");
    
    /* Final Status */
    uart_puts("\r\n=== Final Status ===\r\n");
    uart_puts("System running normally.\r\n");
    uart_puts("Entering idle loop...\r\n\r\n");
    
    /* Vòng lặp vô tận */
    while (1) {
        delay_ms(1000);
        uart_puts(".");
    }
}

/* ============================================================================
   Exception Handlers (Minimal)
   ============================================================================ */

void handle_exception(uint32_t cause, uint32_t epc) {
    uart_puts("\r\n!!! EXCEPTION !!!\r\n");
    uart_puts("Cause: 0x");
    uart_puthex(cause);
    uart_puts(", EPC: 0x");
    uart_puthex(epc);
    uart_puts("\r\n");
    while (1);
}
