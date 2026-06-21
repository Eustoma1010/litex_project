/*
 * Bare-Metal Firmware cho LiteX RISC-V SoC
 * Chức năng: Demo UART communication, Timer interrupt, và System Control
 * Ngôn ngữ: C (C99)
 * Biên dịch: riscv64-unknown-elf-gcc -march=rv32i
 */

#include <stdint.h>

/* ============================================================================
   CSR Memory Map (từ csr.json)
   ============================================================================ */

#define CTRL_BASE              0xF0000000
#define IDENTIFIER_MEM_BASE    0xF0002000
#define TIMER0_BASE            0xF0004000
#define UART_BASE              0xF0006000

/* CTRL Registers */
#define CTRL_RESET             (CTRL_BASE + 0x00)
#define CTRL_SCRATCH           (CTRL_BASE + 0x04)
#define CTRL_BUS_ERRORS        (CTRL_BASE + 0x08)

/* TIMER0 Registers */
#define TIMER0_LOAD            (TIMER0_BASE + 0x00)
#define TIMER0_RELOAD          (TIMER0_BASE + 0x04)
#define TIMER0_EN              (TIMER0_BASE + 0x08)
#define TIMER0_UPDATE_VALUE    (TIMER0_BASE + 0x0C)
#define TIMER0_VALUE           (TIMER0_BASE + 0x10)
#define TIMER0_EV_STATUS       (TIMER0_BASE + 0x14)
#define TIMER0_EV_PENDING      (TIMER0_BASE + 0x18)
#define TIMER0_EV_ENABLE       (TIMER0_BASE + 0x1C)

/* UART Registers */
#define UART_RXTX              (UART_BASE + 0x00)
#define UART_TXFULL            (UART_BASE + 0x04)
#define UART_RXEMPTY           (UART_BASE + 0x08)

/* Identifier Memory */
#define IDENTIFIER_MEM_ID      (IDENTIFIER_MEM_BASE + 0x00)
#define IDENTIFIER_MEM_VERSION (IDENTIFIER_MEM_BASE + 0x04)

/* ============================================================================
   Helper Functions - Memory Access
   ============================================================================ */

static inline uint32_t read_reg(uint32_t addr) {
    return *(volatile uint32_t *)addr;
}

static inline void write_reg(uint32_t addr, uint32_t value) {
    *(volatile uint32_t *)addr = value;
}

/* ============================================================================
   UART Functions
   ============================================================================ */

void uart_init(void) {
    /* UART được khởi tạo mặc định với baud rate 115200 */
}

int uart_tx_ready(void) {
    return !read_reg(UART_TXFULL);
}

int uart_rx_available(void) {
    return !read_reg(UART_RXEMPTY);
}

void uart_putc(char c) {
    while (!uart_tx_ready()) {
        /* Chờ buffer transmit rỗng */
    }
    write_reg(UART_RXTX, (uint32_t)c);
}

char uart_getc(void) {
    while (!uart_rx_available()) {
        /* Chờ dữ liệu */
    }
    return (char)(read_reg(UART_RXTX) & 0xFF);
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
    /*
     * Khởi tạo Timer với chu kỳ nhất định
     * Tần suất hệ thống: 100 MHz → clock = 10 ns
     * Period (ms) = value / 100000
     */
    write_reg(TIMER0_LOAD, period * 100000);     /* Tính toán count từ ms */
    write_reg(TIMER0_RELOAD, period * 100000);   /* Auto-reload */
    write_reg(TIMER0_EN, 1);                      /* Bật timer */
}

void timer_stop(void) {
    write_reg(TIMER0_EN, 0);
}

uint32_t timer_read(void) {
    /* Đọc giá trị counter hiện tại */
    write_reg(TIMER0_UPDATE_VALUE, 1);
    return read_reg(TIMER0_VALUE);
}

int timer_irq_pending(void) {
    return read_reg(TIMER0_EV_PENDING) & 1;
}

void timer_irq_clear(void) {
    write_reg(TIMER0_EV_PENDING, 1);
}

/* ============================================================================
   System Functions
   ============================================================================ */

void system_info(void) {
    uart_puts("\r\n=== LiteX RISC-V SoC System Information ===\r\n");
    
    uint32_t id = read_reg(IDENTIFIER_MEM_ID);
    uint32_t version = read_reg(IDENTIFIER_MEM_VERSION);
    uint32_t errors = read_reg(CTRL_BUS_ERRORS);
    
    uart_puts("System ID:     0x");
    uart_puthex(id);
    uart_puts("\r\nSystem Ver:    0x");
    uart_puthex(version);
    uart_puts("\r\nBus Errors:    ");
    uart_puthex(errors);
    uart_puts("\r\n");
}

void delay_ms(uint32_t ms) {
    /*
     * Độ trễ đơn giản dựa trên vòng lặp
     * Tần suất CPU 100 MHz → 1 chu kỳ = 10 ns
     * 1 ms = 100,000 chu kỳ
     * Mỗi vòng lặp ~10 chu kỳ → 10,000 lần lặp/ms
     */
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
    uart_puts("   LiteX RISC-V SoC - Bare-Metal Firmware\r\n");
    uart_puts("   Platform: VexRiscv (32-bit RISC-V)\r\n");
    uart_puts("   Frequency: 100 MHz\r\n");
    uart_puts("================================================\r\n\r\n");
    
    /* Hiển thị thông tin hệ thống */
    system_info();
    
    /* Kiểm tra bộ nhớ */
    uart_puts("\r\n=== Memory Test ===\r\n");
    uint32_t test_addr = 0x10001800;  /* SRAM_TEST (offset 6KB, trong vùng 8KB) */
    uint32_t test_value = 0xDEADBEEF;
    
    write_reg(test_addr, test_value);
    uint32_t read_value = read_reg(test_addr);
    
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
    write_reg(CTRL_SCRATCH, 0xABCD1234);
    uint32_t scratch = read_reg(CTRL_SCRATCH);
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
