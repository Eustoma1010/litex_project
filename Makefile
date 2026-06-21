# Makefile cho LiteX RISC-V Firmware
# Hỗ trợ 2 luồng:
# 1. firmware.c: Map địa chỉ bằng tay (Thủ công)
# 2. firmware_csr.c: Map địa chỉ tự động qua csr.h (Từ cấu hình LiteX)
SHELL := /bin/bash

CROSS_COMPILE ?= riscv64-unknown-elf-
CC       = $(CROSS_COMPILE)gcc
AS       = $(CROSS_COMPILE)as
LD       = $(CROSS_COMPILE)ld
OBJCOPY  = $(CROSS_COMPILE)objcopy
OBJDUMP  = $(CROSS_COMPILE)objdump

# Flags - 32-bit RISC-V RV32I with soft ABI
CFLAGS   = -I build/sim/software/include -I my_includes -march=rv32i -mabi=ilp32 -mcmodel=medany -ffunction-sections -fdata-sections -Wall -fno-builtin -nostdlib -O2
ASFLAGS  = -march=rv32i
LDFLAGS  = -T linker.ld -nostdlib -melf32lriscv -gc-sections

# Files for Manual mapping flow
SRCS1     = firmware.c
ASMS      = crt0.s
OBJS1     = $(SRCS1:.c=.o) $(ASMS:.s=.o)
TARGET1   = firmware.elf
BINARY1   = firmware.bin

# Files for Auto CSR mapping flow
SRCS2     = firmware_csr.c
OBJS2     = $(SRCS2:.c=.o) $(ASMS:.s=.o)
TARGET2   = firmware_csr.elf
BINARY2   = firmware_csr.bin

# Rules
.PHONY: all clean build

all: $(BINARY1) $(BINARY2)

# Build manual firmware
$(TARGET1): $(OBJS1)
	@echo "[LD] Linking $@..."
	$(LD) $(LDFLAGS) -Map=firmware.map -o $@ $^

# Build auto csr firmware
$(TARGET2): $(OBJS2)
	@echo "[LD] Linking $@..."
	$(LD) $(LDFLAGS) -Map=firmware_csr.map -o $@ $^

%.o: %.c
	@echo "[CC] Compiling $<..."
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	@echo "[AS] Assembling $<..."
	$(AS) $(ASFLAGS) -o $@ $<

%.bin: %.elf
	@echo "[OBJCOPY] Converting to binary $@..."
	$(OBJCOPY) -O binary $< $@
	@ls -lh $@

clean:
	@echo "[CLEAN] Removing build artifacts..."
	rm -f *.o *.elf *.bin *.map *.dump *.log

build: clean all

wave:
	@echo "[WAVE] Mở trình xem dạng sóng GtkWave..."
	@if [ -f build/sim/gateware/sim.fst ]; then \
		gtkwave build/sim/gateware/sim.fst signals.gtkw & \
	else \
		echo "Lỗi: Không tìm thấy sim.fst. Hãy chạy litex_sim với cờ --trace-fst trước!"; \
	fi

# Chạy nhanh: Nhúng firmware thẳng vào ROM, bỏ qua BIOS, boot tức thì
fast: clean all
	@echo "[FAST] Nhúng firmware vào ROM và khởi chạy mô phỏng..."
	source venv/bin/activate && litex_sim \
		--cpu-type=vexriscv \
		--integrated-rom-init firmware_csr.bin
