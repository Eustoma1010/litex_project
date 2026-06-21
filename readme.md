# Báo Cáo Kỹ Thuật: Framework LiteX — Tự Động Hóa Thiết Kế System-on-Chip

> **Repository:** [https://github.com/enjoy-digital/litex.git](https://github.com/enjoy-digital/litex.git)  
> **CPU:** VexRiscv (RV32I, 32-bit RISC-V soft-core)  
> **Toolchain:** Verilator (mô phỏng), GCC RISC-V (cross-compile), GtkWave (waveform)

---

## Mục Lục

1. [Giới thiệu LiteX](#1-giới-thiệu-litex)
2. [So sánh luồng thiết kế SoC: Truyền thống vs. LiteX](#2-so-sánh-luồng-thiết-kế-soc-truyền-thống-vs-litex)
3. [Phân tích sự tối ưu qua từng bước thiết kế](#3-phân-tích-sự-tối-ưu-qua-từng-bước-thiết-kế)
4. [Kiến trúc hệ thống SoC được sinh ra](#4-kiến-trúc-hệ-thống-soc-được-sinh-ra)
5. [Cấu trúc mã nguồn dự án](#5-cấu-trúc-mã-nguồn-dự-án)
6. [Phân tích chi tiết từng thành phần](#6-phân-tích-chi-tiết-từng-thành-phần)
7. [Hướng dẫn chạy từ đầu (Step-by-Step)](#7-hướng-dẫn-chạy-từ-đầu-step-by-step)
8. [Phân tích dạng sóng (Waveform)](#8-phân-tích-dạng-sóng-waveform)

---

## 1. Giới thiệu LiteX

LiteX là một framework mã nguồn mở được viết bằng Python (dựa trên thư viện Migen), cho phép mô tả, tích hợp và sinh tự động các hệ thống System-on-Chip (SoC) hoàn chỉnh.

**Điểm khác biệt cốt lõi:** Thay vì viết Verilog/VHDL thủ công để mô tả phần cứng, kỹ sư chỉ cần viết vài dòng Python khai báo cấu hình. LiteX sẽ tự động:
- Sinh mã RTL Verilog (`sim.v`) cho toàn bộ SoC.
- Nối Bus giữa CPU và các ngoại vi (Wishbone interconnect).
- Phân bổ không gian địa chỉ bộ nhớ (Memory Map).
- Sinh file header C (`csr.h`) chứa các macro truy cập thanh ghi, khớp 100% với RTL.
- Biên dịch BIOS khởi động và đóng gói trình mô phỏng qua Verilator.

---

## 2. So sánh luồng thiết kế SoC: Truyền thống vs. LiteX

| Bước | Luồng truyền thống | Luồng LiteX |
|---|---|---|
| **1. Thiết kế IP** | Viết mã Verilog/VHDL thủ công cho từng module (RAM, UART, CPU) hoặc sử dụng IP độc quyền của hãng (Xilinx IP Catalog, Intel QSYS). | Gọi các IP Core mã nguồn mở có sẵn (LiteDRAM, LiteEth, VexRiscv) thông qua các lớp Python. Ví dụ: `soc.add_uart("uart", baudrate=115200)`. |
| **2. Tích hợp (Wiring)** | Nối dây thủ công từng tín hiệu Bus (AXI/Wishbone/Avalon) giữa CPU và ngoại vi. Phải tự quy hoạch bản đồ địa chỉ. Rất dễ sai sót. | Khai báo thành phần trong Python. LiteX tự sinh RTL kết nối các Bus (Wishbone Shared Interconnect) và phân bổ không gian địa chỉ tự động. |
| **3. Đồng bộ HW/SW** | Kỹ sư Firmware (C) phải tự tra cứu tài liệu RTL do kỹ sư phần cứng viết, rồi tự định nghĩa thủ công `#define UART_BASE 0xF0006000`. Nếu RTL thay đổi, phải sửa lại bằng tay. | LiteX tự động sinh file `csr.h` chứa toàn bộ macro và hàm truy cập thanh ghi (CSR). RTL thay đổi → Header C tự cập nhật theo. |
| **4. Biên dịch (Build)** | Phải mở GUI (Vivado/Quartus), tạo project, gán pin, viết file ràng buộc (XDC/SDC), click chạy thủ công từng bước Synth → Place → Route. | LiteX chạy toàn bộ bằng một dòng lệnh CLI: `python3 -m litex_boards.targets.digilent_arty --build`. Tự sinh file TCL và gọi ngầm Vivado/Yosys. |
| **5. Nạp & Chạy** | Nạp bitstream qua JTAG. Phải cấu hình riêng một quy trình phức tạp để nạp firmware C vào bộ nhớ (JTAG-to-AXI, file ảnh, debug probe). | Nạp bitstream. Biên dịch C bằng `make build`, nạp firmware trực tiếp qua UART/Ethernet bằng lệnh `litex_term --kernel firmware.bin`. |

---

## 3. Phân tích sự tối ưu qua từng bước thiết kế

### 3.1. Tối ưu khâu Thiết kế IP (Bước 1)

LiteX cung cấp sẵn một hệ sinh thái các khối IP mã nguồn mở được viết hoàn toàn dưới dạng Python. Khác với quy trình truyền thống (phải viết hàng ngàn dòng mã trạng thái Verilog và khai báo tín hiệu), LiteX cho phép khởi tạo IP cực kỳ ngắn gọn.

**Các thư viện IP tiêu biểu trong hệ sinh thái LiteX:**

| Thư viện | Chức năng | Ví dụ lệnh Python |
|---|---|---|
| **LiteDRAM** | Bộ điều khiển SDRAM/DDR | `self.add_sdram("sdram", phy=sdrphy, module=IS42S16160(...))` |
| **LiteEth** | Giao tiếp Ethernet MAC/PHY | `self.add_ethernet(phy=ethphy)` |
| **LiteSPI** | Giao tiếp SPI Flash | `self.add_spi_flash(mode="4x")` |
| **LiteSDCard** | Đọc/ghi thẻ SD | `self.add_sdcard()` |
| **LiteSATA** | Giao tiếp SATA HDD/SSD | `self.add_sata(phy=sataphy)` |
| **LitePCIe** | Giao tiếp PCIe | `self.add_pcie(phy=pciephy)` |
| **LiteScope** | Logic Analyzer (Debug) | `self.add_scope(analyzer_depth=1024)` |

**So sánh code tạo bộ điều khiển SDRAM:**

Luồng truyền thống (Verilog) — cần hàng trăm dòng code căn chỉnh timing:
```verilog
// Phải tự viết state machine cho các pha: PRECHARGE, ACTIVATE, READ, WRITE, REFRESH
// Phải tự tính toán CAS Latency, tRCD, tRP, tRAS cho từng loại chip cụ thể
module sdram_controller (
    input wire clk, rst,
    input wire [24:0] addr,
    // ... hàng trăm dòng tín hiệu và logic
);
```

Luồng LiteX (Python) — chỉ cần 3 dòng:
```python
from litedram.modules import IS42S16160   # Import cấu hình chip SDRAM
from litedram.phy import GENSDRPHY

sdrphy = GENSDRPHY(pads)                  # Tạo lớp Physical Layer
self.add_sdram("sdram", phy=sdrphy, module=IS42S16160(sys_clk_freq))
# LiteX tự sinh hàng ngàn dòng Verilog tương ứng, bao gồm cả timing!
```

### 3.2. Tối ưu khâu Tích hợp Wiring (Bước 2)

Sau khi cấu hình trên Python, LiteX tự động nối dây giữa các linh kiện (CPU, RAM, UART) qua hệ thống Bus Wishbone, tự phân bổ vùng nhớ (Address Mapping), và xuất ra mã Verilog.

**Đoạn RTL tự sinh trong file `sim.v`:**
```verilog
// Module sim — Top-Level tự sinh bởi LiteX (không cần viết tay)
module sim (
    input  wire    [7:0] serial_sink_data,     // Dữ liệu nhận UART (RX)
    output wire          serial_sink_ready,     // Cờ sẵn sàng nhận
    input  wire          serial_sink_valid,     // Dữ liệu nhận hợp lệ
    output wire    [7:0] serial_source_data,    // Dữ liệu phát UART (TX)
    input  wire          serial_source_ready,   // Cờ sẵn sàng phát
    output wire          serial_source_valid,   // Dữ liệu phát hợp lệ
    input  wire          sys_clk,               // Xung nhịp hệ thống
    input  wire          sys_rst                // Tín hiệu Reset
);
```

Toàn bộ mạng đấu nối Wishbone (Arbiter, Decoder, Timeout) đều được sinh tự động:
```
SoC Hierarchy (tự sinh):
├── bus → InterconnectShared
│         ├── arbiter → RoundRobin     ← Phân xử quyền truy cập Bus
│         ├── decoder → Decoder        ← Giải mã địa chỉ → chọn Slave
│         └── timeout → WaitTimer      ← Phát hiện Bus treo
```

Trong thiết kế truyền thống, kỹ sư phải tự viết tay toàn bộ logic arbiter, decoder, address decoder, timeout — đây là công việc cực kỳ tốn thời gian và dễ sai sót.

### 3.3. Tối ưu khâu Đồng bộ HW/SW (Bước 3)

Dự án này cung cấp 2 luồng firmware song song để chứng minh trực quan giá trị của cơ chế tự sinh CSR:

- **Luồng thủ công (`firmware.c`):** Nếu thêm 1 ngoại vi vào SoC, kỹ sư phải tự tra cứu lại toàn bộ bản đồ địa chỉ và sửa từng dòng `#define` bằng tay. Sai 1 bit → hệ thống sẽ crash.
- **Luồng tự động (`firmware_csr.c`):** Chỉ cần gọi hàm `uart_rxtx_write()` do `csr.h` cung cấp. Khi phần cứng thay đổi, chạy lại `litex_sim` là file `csr.h` tự cập nhật — firmware không cần sửa dù chỉ một dòng code.

Cơ chế này còn mở rộng ra cả Linker Script: file `linker.ld` sử dụng `INCLUDE regions.ld` (file tự sinh) thay vì khai báo cứng vùng nhớ. Nhờ vậy, toàn bộ chuỗi công cụ (Compiler → Linker → Loader) đều tự thích ứng theo phần cứng.

### 3.4. Tối ưu khâu Biên dịch Build (Bước 4)

LiteX không yêu cầu mở phần mềm đồ họa nặng nề. Toàn bộ quy trình Synthesis → Place & Route → Bitstream Generation được gói gọn trong một dòng lệnh CLI:

```bash
python3 -m litex_boards.targets.digilent_arty --build --toolchain vivado
```

Khi thực thi, LiteX sẽ:
1. Sinh mã RTL Verilog từ Python.
2. Tự động viết file Script TCL (`.tcl`) cho Vivado.
3. Gọi Vivado ở chế độ Batch Mode (không GUI) để chạy toàn bộ Synth → Place → Route.
4. Xuất file Bitstream `.bit` sẵn sàng nạp xuống bo mạch.

### 3.5. Tối ưu khâu Nạp & Chạy (Bước 5)

LiteX thống nhất quy trình nạp firmware cho cả mô phỏng ảo lẫn phần cứng thật bằng cùng một tool `litex_term`:

| Ngữ cảnh | Lệnh nạp |
|---|---|
| Mô phỏng (Verilator) | `litex_term --kernel firmware.bin socket://localhost:1234` |
| Bo mạch thật (USB) | `litex_term --kernel firmware.bin /dev/ttyUSB0` |

Kỹ sư chỉ cần đổi cổng kết nối, toàn bộ giao thức nạp (Serial Boot Protocol) được giữ nguyên. Điều này rút ngắn khoảng cách giữa Simulation và Hardware Deployment xuống gần bằng không.

---

## 4. Kiến trúc hệ thống SoC được sinh ra

Khi chạy lệnh `litex_sim --cpu-type=vexriscv`, LiteX tự động sinh ra một SoC hoàn chỉnh với cấu trúc phân cấp như sau:

```
SimSoC
├── crg (CRG)                      ← Clock/Reset Generator
├── bus (SoCBusHandler)             ← Quản lý Bus Wishbone 32-bit
│    └── InterconnectShared         ← Mạng đấu nối chia sẻ (Arbiter + Decoder)
├── cpu (VexRiscv)                  ← CPU RISC-V 32-bit
├── rom (SRAM)                      ← ROM chứa BIOS (0x00000000)
├── sram (SRAM)                     ← RAM nội bộ (0x10000000, 8KB)
├── uart (UART)                     ← Ngoại vi giao tiếp Serial
│    ├── phy (RS232PHYModel)        ← Lớp vật lý UART
│    ├── tx_fifo / rx_fifo          ← Bộ đệm FIFO truyền/nhận
│    └── ev (EventManager)          ← Quản lý ngắt UART
├── timer0 (Timer)                  ← Bộ hẹn giờ hệ thống
├── csr_bridge (Wishbone2CSR)       ← Cầu nối Bus Wishbone → CSR
└── csr_bankarray (CSRBankArray)    ← Mảng thanh ghi điều khiển
```

**Bản đồ bộ nhớ (Memory Map):**

| Vùng | Địa chỉ bắt đầu | Kích thước | Chế độ | Mô tả |
|---|---|---|---|---|
| `rom`  | `0x00000000` | 128 KB | RX | ROM chứa BIOS khởi động |
| `sram` | `0x10000000` | 8 KB   | RWX | SRAM nội bộ tốc độ cao |
| `csr`  | `0xF0000000` | 64 KB  | RW | Không gian thanh ghi ngoại vi |

**Phân bổ địa chỉ CSR (thanh ghi ngoại vi):**

| Ngoại vi | Địa chỉ cơ sở | Chức năng |
|---|---|---|
| `ctrl` | `0xF0000000` | Điều khiển hệ thống (Reset, Scratch, Bus Errors) |
| `identifier_mem` | `0xF0002000` | Chuỗi nhận dạng SoC |
| `timer0` | `0xF0004000` | Timer/Counter 32-bit |
| `uart` | `0xF0006000` | UART TX/RX, trạng thái FIFO |

---

## 5. Cấu trúc mã nguồn dự án

Các file dưới đây là **do chúng ta tự viết** (được đẩy lên Git), tất cả các thư viện LiteX và file build đều được liệt kê trong `.gitignore`:

```
.
├── readme.md              ← Tài liệu này (Báo cáo + Hướng dẫn)
├── Makefile               ← Biên dịch firmware + mở GtkWave
├── firmware.c             ← Firmware C — Luồng thủ công (hardcode địa chỉ)
├── firmware_csr.c         ← Firmware C — Luồng tự động (dùng generated/csr.h)
├── crt0.s                 ← Mã khởi động Assembly (RISC-V startup code)
├── linker.ld              ← Script liên kết (tự động INCLUDE regions.ld)
├── signals.gtkw           ← Cấu hình tín hiệu sẵn cho GtkWave
├── my_includes/system.h   ← File header hỗ trợ biên dịch
├── .gitignore             ← Loại bỏ thư viện, build, binaries khỏi Git
└── build/                 ← [Tự động sinh] Phần cứng + Header C
    └── sim/
        ├── gateware/sim.v         ← RTL Verilog tự động sinh
        ├── gateware/sim.fst       ← Waveform (khi dùng --trace-fst)
        ├── csr.json               ← Bản đồ thanh ghi (JSON)
        └── software/include/
            └── generated/
                ├── csr.h          ← Header C tự động sinh
                └── regions.ld     ← Bản đồ bộ nhớ cho Linker
```

---

## 6. Phân tích chi tiết từng thành phần

### 6.1. Hai luồng Firmware — Chứng minh sức mạnh đồng bộ HW/SW

Dự án cung cấp song song 2 file firmware để minh họa rõ ràng sự khác biệt giữa cách tiếp cận truyền thống và cách tiếp cận LiteX:

**Luồng 1 — Thủ công (`firmware.c`):**
Kỹ sư phải tự tra cứu file `csr.json` rồi định nghĩa bằng tay:
```c
/* Khai báo cứng — Nếu phần cứng thay đổi, phải sửa lại bằng tay */
#define UART_BASE   0xF0006000
#define UART_RXTX   (UART_BASE + 0x00)
#define UART_TXFULL (UART_BASE + 0x04)

/* Ghi dữ liệu vào thanh ghi bằng con trỏ volatile */
void uart_putc(char c) {
    while (*(volatile uint32_t *)UART_TXFULL);   // Chờ buffer rỗng
    *(volatile uint32_t *)UART_RXTX = (uint32_t)c;  // Ghi ký tự
}
```

**Luồng 2 — Tự động (`firmware_csr.c`):**
Chỉ cần include file header do LiteX tự sinh, mọi địa chỉ đều được hệ thống tự quản lý:
```c
/* Khai báo bộ truy cập thanh ghi cấp thấp */
static inline uint32_t csr_read_simple(uint32_t addr) {
    return *(volatile uint32_t *)addr;
}
static inline void csr_write_simple(uint32_t value, uint32_t addr) {
    *(volatile uint32_t *)addr = value;
}

#define CSR_ACCESSORS_DEFINED
#include <generated/csr.h>    /* Header tự sinh — Địa chỉ luôn khớp 100% với RTL */

/* Gọi hàm API do LiteX cung cấp, không cần biết địa chỉ cụ thể */
void uart_putc(char c) {
    while (uart_txfull_read());    // Hàm tự sinh từ csr.h
    uart_rxtx_write((uint32_t)c);  // Hàm tự sinh từ csr.h
}
```

### 6.2. Linker Script — Kế thừa tự động bản đồ bộ nhớ

Thay vì khai báo cứng vùng nhớ, file `linker.ld` sử dụng lệnh `INCLUDE` để kế thừa trực tiếp bản đồ bộ nhớ mà LiteX tự sinh ra:

```ld
/* Bao gồm bản đồ bộ nhớ (MEMORY) tự động sinh bởi LiteX */
INCLUDE build/sim/software/include/generated/regions.ld

SECTIONS {
    .text : { *(.text*) } > sram    /* Tên 'sram' do regions.ld định nghĩa */
    .data : { *(.data*) } > sram
    .bss  : { *(.bss*)  } > sram
    .stack : { . += 0x1000; _stack_top = .; } > sram
}
```

Nhờ vậy, khi cấu hình phần cứng thay đổi (ví dụ mở rộng SRAM lên 64KB), Linker Script không cần sửa lại — nó sẽ tự lấy giá trị mới từ `regions.ld`.

### 6.3. Startup Code (`crt0.s`) — Khởi tạo CPU RISC-V

File Assembly này chạy đầu tiên khi CPU được cấp nguồn, trước khi nhảy vào hàm `main()` của C:

```asm
_start:
    la  sp, _stack_top      /* 1. Thiết lập con trỏ Stack */
    /* 2. Xóa vùng BSS (biến chưa khởi tạo) về 0 */
    la  t0, _bss
    la  t1, _ebss
clear_bss:
    beq t0, t1, done
    sw  zero, 0(t0)
    addi t0, t0, 4
    j   clear_bss
done:
    /* 3. Nhảy vào main() */
    j   main
```

### 6.4. Makefile — Tự động hóa biên dịch

Makefile hỗ trợ 3 lệnh chính:

| Lệnh | Chức năng |
|---|---|
| `make build` | Xóa file cũ, biên dịch cả 2 firmware ra `firmware.bin` và `firmware_csr.bin` |
| `make clean` | Xóa các file `.o`, `.elf`, `.bin` (không xóa thư mục `build/` của phần cứng) |
| `make wave`  | Mở GtkWave với file cấu hình `signals.gtkw` đã add sẵn các tín hiệu quan trọng |

---

## 7. Hướng dẫn chạy từ đầu (Step-by-Step)

### Bước 1: Cài đặt môi trường

```bash
sudo apt update
sudo apt install -y build-essential git python3 python3-pip verilator gtkwave curl \
                    libevent-dev libjson-c-dev
sudo apt install -y gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf
```

Khởi tạo thư mục dự án và cài đặt LiteX:
```bash
mkdir -p /data/worktime/litex && cd /data/worktime/litex
python3 -m venv venv
source venv/bin/activate

wget https://raw.githubusercontent.com/enjoy-digital/litex/master/litex_setup.py
chmod +x litex_setup.py
./litex_setup.py --init --install
```

### Bước 2: Sinh phần cứng (Hardware Generation)

Lệnh dưới đây sẽ kích hoạt LiteX tự động thực hiện toàn bộ 3 bước đầu tiên trong bảng so sánh ở Mục 2 (Thiết kế IP → Tích hợp Wiring → Sinh Header C):

```bash
litex_sim --cpu-type=vexriscv --trace-fst --doc
```

Sau khi chạy xong, các file quan trọng được sinh ra:
- `build/sim/gateware/sim.v` — Mã Verilog hoàn chỉnh của SoC.
- `build/sim/software/include/generated/csr.h` — Header C cho firmware.
- `build/sim/software/include/generated/regions.ld` — Bản đồ bộ nhớ cho Linker.
- `build/sim/csr.json` — Bản đồ thanh ghi dạng JSON.

### Bước 3: Biên dịch Firmware (Software Build)

```bash
make build
```

Lệnh này biên dịch đồng thời cả 2 luồng firmware:
- `firmware.c` → `firmware.bin` (luồng thủ công)
- `firmware_csr.c` → `firmware_csr.bin` (luồng tự động CSR)

### Bước 4: Mô phỏng và nạp Firmware qua Socket (UART-TCP)

Mở **2 Terminal** song song:

**Terminal 1 — Khởi chạy SoC mô phỏng:**
```bash
source venv/bin/activate
litex_sim --cpu-type=vexriscv --uart-tcp --trace-fst
```
- Cờ `--uart-tcp`: Mở cổng Socket TCP `localhost:1234` đóng vai trò UART Serial.
- Cờ `--trace-fst`: Ghi lại toàn bộ dạng sóng vào file `sim.fst`.
- Màn hình sẽ hiện `<DUMP ON>` và đứng chờ kết nối (đây là trạng thái bình thường, SoC đang chạy).

**Terminal 2 — Nạp firmware vào SoC:**
```bash
source venv/bin/activate
litex_term --kernel firmware_csr.bin --kernel-adr 0x10000000 socket://localhost:1234
```
- `--kernel firmware_csr.bin`: File firmware cần nạp (có thể thay bằng `firmware.bin` để nạp luồng thủ công).
- `--kernel-adr 0x10000000`: Nạp firmware vào đúng vùng SRAM (khớp với Linker).
- `socket://localhost:1234`: Kết nối qua Socket TCP tới SoC ảo.

> **Lưu ý:** Nếu Terminal hiện ra dấu nhắc `litex>` mà chưa tự nạp, hãy gõ lệnh `serialboot` rồi nhấn Enter để kích hoạt quá trình boot.

### Bước 5: Xem dạng sóng (Waveform)

Sau khi firmware chạy xong, quay lại Terminal 1 nhấn `Ctrl+C` để dừng mô phỏng. File `sim.fst` đã được ghi xong. Chạy:

```bash
make wave
```

Lệnh này tự động mở GtkWave và load file cấu hình `signals.gtkw`, trong đó đã add sẵn các tín hiệu quan trọng (xem Mục 8).

### Bước 6: Nạp xuống phần cứng thực tế (FPGA)

Khi đã kiểm tra thành công trên mô phỏng, thay thế bước Simulation bằng lệnh sinh Bitstream cho bo mạch thật. Ví dụ với Digilent Arty (FPGA Xilinx):
```bash
python3 -m litex_boards.targets.digilent_arty --build --toolchain vivado
```

Nạp firmware qua cáp USB (cùng tool `litex_term`, chỉ đổi cổng kết nối):
```bash
litex_term --kernel firmware_csr.bin /dev/ttyUSB0
```

---

## 8. Phân tích dạng sóng (Waveform)

File `signals.gtkw` đã cấu hình sẵn các nhóm tín hiệu quan trọng để GtkWave tự động hiển thị khi chạy `make wave`:

| Nhóm | Tín hiệu | Ý nghĩa |
|---|---|---|
| **Hệ thống** | `TOP.sim.sys_clk` | Xung nhịp đồng hồ hệ thống (System Clock) |
| | `TOP.sim.sys_rst` | Cờ khởi tạo lại hệ thống (System Reset) |
| **Bus Lệnh (Instruction)** | `TOP.sim.ibus_cyc` | CPU yêu cầu truy cập Bus lệnh (CYCle) |
| | `TOP.sim.ibus_ack` | Bus phản hồi đã sẵn sàng (ACKnowledge) |
| **Bus Dữ Liệu (Data)** | `TOP.sim.dbus_cyc` | CPU yêu cầu đọc/ghi dữ liệu |
| | `TOP.sim.dbus_ack` | Bus dữ liệu phản hồi hoàn tất |
| **Giao tiếp UART** | `TOP.sim.serial_source_valid` | Có dữ liệu hợp lệ đang được truyền đi (TX) |
| | `TOP.sim.serial_sink_valid` | Có dữ liệu hợp lệ đang được nhận về (RX) |

**Cách đọc dạng sóng:**
- Khi `sys_rst` chuyển từ `1 → 0`: Hệ thống thoát trạng thái Reset, CPU bắt đầu nạp lệnh.
- Khi `ibus_cyc` và `ibus_ack` nhấp nháy liên tục: CPU đang nạp lệnh từ ROM/SRAM.
- Khi `dbus_cyc` và `dbus_ack` nhấp nháy: CPU đang đọc/ghi dữ liệu (ví dụ ghi vào UART register).
- Khi `serial_source_valid = 1`: UART đang truyền ký tự ra ngoài (ví dụ: in chữ "Hello").

---

## 9. Tác giả & Đóng góp

**Tác giả chính:**
- Phan Phước Quốc Thiện - [thien3phan370@gmail.com](mailto:thien3phan370@gmail.com)

**Những người đóng góp:**
- Lê Nhật Minh - [24161316@student.hcmute.edu.vn](mailto:24161316@student.hcmute.edu.vn)
- Nguyễn Thanh Thắng - [24161403@student.hcmute.edu.vn](mailto:24161403@student.hcmute.edu.vn)
- Phan Phước Quốc Thiện - [24161408@student.hcmute.edu.vn](mailto:24161408@student.hcmute.edu.vn)
- Nguyễn Văn Thực - [24161421@student.hcmute.edu.vn](mailto:24161421@student.hcmute.edu.vn)
- Bùi Quang Hiệu - [24161236@student.hcmute.edu.vn](mailto:24161236@student.hcmute.edu.vn)
- Huỳnh Tấn Sang - [24161384@student.hcmute.edu.vn](mailto:24161384@student.hcmute.edu.vn)
