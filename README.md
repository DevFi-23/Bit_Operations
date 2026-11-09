# bitops

**English** | [Tiếng Việt](#tiếng-việt)

A small, header-only C library of bit and bit-field macros for 32-bit registers, written for the STM32F103C8T6 (ARM Cortex-M3).

Thư viện C header-only nhỏ gọn gồm các macro thao tác bit và bit-field cho thanh ghi 32 bit, viết cho STM32F103C8T6 (ARM Cortex-M3).

---

## English

[Jump to the Vietnamese version](#tiếng-việt)

`bitops` is a header-only library for **32-bit registers**, written for the **STM32F103C8T6** (ARM Cortex-M3) and usable on any target with a 32-bit `uint32_t`.

It gives you safe, readable replacements for the hand-written `|=`, `&= ~`, `^=` and shift-and-mask expressions that show up all over register-level firmware, and it avoids the undefined behaviour that comes with out-of-range shifts.

- Header-only: copy `include/bitops.h` into your project, no build step.
- No dependencies beyond `<stdint.h>`.
- C99, no compiler extensions.
- Out-of-range arguments are rejected instead of invoking undefined behaviour.
- Covered by an exhaustive host-side unit test suite (over 17,000 assertions).

### Table of contents

- [Quick start](#quick-start)
- [API reference](#api-reference)
- [Behaviour on invalid arguments](#behaviour-on-invalid-arguments)
- [Usage examples (STM32F103)](#usage-examples-stm32f103)
- [Design notes](#design-notes)
- [Limitations](#limitations)
- [Project structure](#project-structure)
- [Building and running the tests](#building-and-running-the-tests)
- [Test strategy](#test-strategy)

### Quick start

```c
#include "bitops.h"

uint32_t reg = 0u;

BIT_SET(reg, 5);                 /* reg = 0x00000020 */
BIT_TGL(reg, 0);                 /* reg = 0x00000021 */
BIT_CLR(reg, 5);                 /* reg = 0x00000001 */
uint32_t b = BIT_READ(reg, 0);   /* b = 1 */

FIELD_SET(reg, 4, 8, 0xABu);     /* write 0xAB into bits [11:4] */
uint32_t f = FIELD_GET(reg, 4, 8);   /* f = 0xAB */
```

### API reference

`reg` is any modifiable 32-bit lvalue (a variable, an array element, a struct member, `*ptr`, `ptr->member`). `pos` is the bit position (0 = LSB, 31 = MSB). `w` is the field width in bits.

#### Single-bit operations

| Macro | Kind | Description |
|---|---|---|
| `BIT_SET(reg, pos)` | statement | Set bit `pos` to 1. |
| `BIT_CLR(reg, pos)` | statement | Clear bit `pos` to 0. |
| `BIT_TGL(reg, pos)` | statement | Toggle bit `pos`. |
| `BIT_READ(reg, pos)` | expression | Returns `1` or `0` (the value of bit `pos`, not a mask). |

#### Bit-field operations

| Macro | Kind | Description |
|---|---|---|
| `MASK(w)` | expression | Returns a mask with the `w` lowest bits set. `MASK(0) == 0`, `MASK(32) == 0xFFFFFFFF`. |
| `FIELD_GET(reg, pos, w)` | expression | Returns the field `reg[pos+w-1 : pos]`, right-aligned. |
| `FIELD_SET(reg, pos, w, val)` | statement | Writes `val` into field `reg[pos+w-1 : pos]`. Bits outside the field are left unchanged. `val` is truncated to `w` bits. |

Statement macros are wrapped in `do { ... } while (0)`, so they are safe inside an unbraced `if` / `else`.

### Behaviour on invalid arguments

Invalid arguments never cause undefined behaviour. They are rejected and the result is neutral:

| Call | Condition | Result |
|---|---|---|
| `BIT_SET` / `BIT_CLR` / `BIT_TGL` | `pos >= 32` | `reg` unchanged |
| `BIT_READ` | `pos >= 32` | `0` |
| `FIELD_GET` | `pos >= 32`, `w > 32`, or `pos + w > 32` | `0` |
| `FIELD_SET` | `pos >= 32`, `w > 32`, or `pos + w > 32` | `reg` unchanged |
| `FIELD_GET` / `FIELD_SET` | `w == 0` | `0` / `reg` unchanged |
| `MASK` | `w >= 32` | `0xFFFFFFFF` |

A negative `pos` is converted to a very large unsigned value and is therefore rejected too.

Because rejection is silent, `FIELD_GET` returning `0` does not tell you whether the field really holds 0 or the arguments were invalid. When the arguments are compile-time constants, a mistake is easy to spot during review; when they are computed at runtime, validate them before calling.

### Usage examples (STM32F103)

The examples below use the raw register layout of the STM32F103 (the "Blue Pill" LED is on PC13).

**Enable the GPIOC clock** (`RCC->APB2ENR`, bit 4 = `IOPCEN`):

```c
BIT_SET(RCC->APB2ENR, 4);
```

**Configure PC13 as a 2 MHz push-pull output.** `GPIOC->CRH` holds `CNF13[1:0]` in bits 23:22 and `MODE13[1:0]` in bits 21:20. Reset value is `0x44444444`:

```c
FIELD_SET(GPIOC->CRH, 20, 2, 2u);   /* MODE13 = 0b10 (output, 2 MHz) */
FIELD_SET(GPIOC->CRH, 22, 2, 0u);   /* CNF13  = 0b00 (push-pull)     */
/* GPIOC->CRH == 0x44244444 */
```

**Toggle the LED and read the pin:**

```c
BIT_TGL(GPIOC->ODR, 13);
if (BIT_READ(GPIOC->IDR, 13)) {
    /* pin is high */
}
```

**Read a multi-bit field** (for example the PLL multiplier in `RCC->CFGR[21:18]`):

```c
uint32_t pllmul = FIELD_GET(RCC->CFGR, 18, 4);
```

### Design notes

- **`do { } while (0)` for statement macros.** The macro expands to a single statement, so `if (x) BIT_SET(r, 1); else BIT_CLR(r, 1);` works without braces.
- **`UINT32_C(1)` for constants.** The literal has a well-defined 32-bit unsigned type regardless of the platform's `int` width, so `1 << 31` is never a signed-overflow case.
- **Bounds checks before every shift.** Shifting a 32-bit value by 32 or more is undefined behaviour in C. Every macro checks `pos` and `w` first, and uses `(pos) & 31u` as a second guard on the shift amount itself.
- **`MASK` special-cases `w == 0` and `w >= 32`.** The general formula `0xFFFFFFFF >> (32 - w)` would shift by 32 when `w == 0`, so those two cases are handled separately.
- **Every macro argument is parenthesised**, so expressions such as `BIT_SET(reg, 2 + 3)` behave as expected.
- **Reads the register once in `FIELD_SET`.** The read-modify-write is a single read followed by a single write of `reg`.

### Limitations

- **32-bit registers only.** `uint64_t` registers are not supported: for example `BIT_CLR` on a 64-bit value would also clear the upper 32 bits. `uint8_t` and `uint16_t` registers are not part of the tested scope either.
- **Macro arguments may be evaluated more than once.** Do not pass expressions with side effects such as `BIT_SET(reg, i++)`.
- **Invalid arguments fail silently** (see the table above); there is no error return or assertion.
- **`MASK(w)` with `w > 32` returns a full mask,** whereas `FIELD_GET` / `FIELD_SET` treat `w > 32` as invalid.
- **Signed `pos` / `w` arguments can trigger `-Wsign-compare`** in the macro's range check. Pass unsigned values or constants to keep builds warning-free.
- **`MASK` is a short, generic name** and can collide with macros from vendor headers. If a clash occurs in your project, rename it (for example to `BITOPS_MASK`) in `bitops.h`.

### Project structure

```
.
├── include/
│   └── bitops.h          # the library (header-only)
├── test/
│   └── test_bitops.c     # unit tests
└── README.md
```

### Building and running the tests

The tests run on a host machine (they do not need the STM32 hardware or a toolchain for ARM). Any C99 compiler works; the command below enables warnings and the undefined-behaviour and address sanitizers:

```sh
gcc -std=c99 -Wall -Wextra -Wpedantic -g -fsanitize=undefined,address \
    test/test_bitops.c -o test_bitops
./test_bitops
```

Expected output:

```
===SUMMARY===
Total       : 17831
Test passed : 17831
Test failed : 0
```

The program exits with status `0` when every test passes and `1` otherwise, so it can be used directly in a CI job or a Makefile target. On failure, each failing assertion prints its file, line, the expression and, for value comparisons, the actual and expected values.

### Test strategy

The suite tests only the public features of the library: `BIT_SET`, `BIT_CLR`, `BIT_TGL`, `BIT_READ`, `MASK`, `FIELD_GET` and `FIELD_SET`.

- **Hand-computed constants** for typical and boundary cases (for example bit 0, bit 31, `0xF78912C4` with bit 7 cleared gives `0xF7891244`).
- **Exhaustive sweeps:** every bit position 0 to 31, every valid `(pos, w)` pair, and every invalid `(pos, w)` pair, run against several bit patterns (all zeros, all ones, alternating bits, and others).
- **Independent reference model.** Expected values are computed with plain C operators and a bit-by-bit `ref_mask()`, not with the macros under test, so a bug in a macro cannot cancel itself out in the check.
- **Properties:** idempotence of set/clear, toggle twice returns the original value, `FIELD_SET` followed by `FIELD_GET` returns `val & MASK(w)`, bits outside a field are untouched, `BIT_READ` and `FIELD_GET` do not modify the register.
- **Invalid input:** `pos` of 32, 33, 100, `0xFFFFFFFF` and `-1`; widths of 33 and above; fields that cross bit 31.
- **Argument forms:** compile-time constants, runtime variables and arithmetic expressions.

The tests are executed on the host, so they verify the logic of the macros. They do not measure generated code size or timing on the Cortex-M3.

---

## Tiếng Việt

[Chuyển sang bản tiếng Anh](#english)

`bitops` là thư viện header-only cho **thanh ghi 32 bit**, viết cho **STM32F103C8T6** (ARM Cortex-M3) và dùng được trên mọi nền tảng có `uint32_t` 32 bit.

Thư viện thay thế các biểu thức `|=`, `&= ~`, `^=` và shift-and-mask viết tay xuất hiện khắp nơi trong firmware thao tác thanh ghi bằng các macro an toàn, dễ đọc, đồng thời tránh hành vi không xác định (undefined behaviour) khi dịch bit vượt dải.

- Header-only: chỉ cần chép `include/bitops.h` vào project, không cần bước build.
- Không phụ thuộc gì ngoài `<stdint.h>`.
- Chuẩn C99, không dùng mở rộng của compiler.
- Tham số ngoài dải bị từ chối thay vì gây ra hành vi không xác định.
- Được phủ bởi bộ unit test chạy trên host, kiểm tra vét cạn (hơn 17.000 phép kiểm tra).

### Mục lục

- [Bắt đầu nhanh](#bắt-đầu-nhanh)
- [Tham chiếu API](#tham-chiếu-api)
- [Hành vi khi tham số không hợp lệ](#hành-vi-khi-tham-số-không-hợp-lệ)
- [Ví dụ sử dụng (STM32F103)](#ví-dụ-sử-dụng-stm32f103)
- [Ghi chú thiết kế](#ghi-chú-thiết-kế)
- [Hạn chế](#hạn-chế)
- [Cấu trúc project](#cấu-trúc-project)
- [Build và chạy test](#build-và-chạy-test)
- [Chiến lược kiểm thử](#chiến-lược-kiểm-thử)

### Bắt đầu nhanh

```c
#include "bitops.h"

uint32_t reg = 0u;

BIT_SET(reg, 5);                 /* reg = 0x00000020 */
BIT_TGL(reg, 0);                 /* reg = 0x00000021 */
BIT_CLR(reg, 5);                 /* reg = 0x00000001 */
uint32_t b = BIT_READ(reg, 0);   /* b = 1 */

FIELD_SET(reg, 4, 8, 0xABu);     /* ghi 0xAB vào bit [11:4] */
uint32_t f = FIELD_GET(reg, 4, 8);   /* f = 0xAB */
```

### Tham chiếu API

`reg` là một lvalue 32 bit có thể sửa đổi (biến, phần tử mảng, member của struct, `*ptr`, `ptr->member`). `pos` là vị trí bit (0 = LSB, 31 = MSB). `w` là độ rộng của trường, tính bằng bit.

#### Thao tác trên từng bit

| Macro | Dạng | Mô tả |
|---|---|---|
| `BIT_SET(reg, pos)` | câu lệnh | Đặt bit `pos` lên 1. |
| `BIT_CLR(reg, pos)` | câu lệnh | Xóa bit `pos` về 0. |
| `BIT_TGL(reg, pos)` | câu lệnh | Đảo bit `pos`. |
| `BIT_READ(reg, pos)` | biểu thức | Trả về `1` hoặc `0` (giá trị của bit `pos`, không phải mask). |

#### Thao tác trên bit-field

| Macro | Dạng | Mô tả |
|---|---|---|
| `MASK(w)` | biểu thức | Trả về mask có `w` bit thấp nhất bằng 1. `MASK(0) == 0`, `MASK(32) == 0xFFFFFFFF`. |
| `FIELD_GET(reg, pos, w)` | biểu thức | Trả về trường `reg[pos+w-1 : pos]`, đã dịch về bit 0. |
| `FIELD_SET(reg, pos, w, val)` | câu lệnh | Ghi `val` vào trường `reg[pos+w-1 : pos]`. Các bit ngoài trường giữ nguyên. `val` bị cắt còn `w` bit. |

Các macro dạng câu lệnh được bọc trong `do { ... } while (0)` nên dùng an toàn trong `if` / `else` không có ngoặc nhọn.

### Hành vi khi tham số không hợp lệ

Tham số không hợp lệ không bao giờ gây ra hành vi không xác định. Chúng bị từ chối và kết quả là giá trị trung tính:

| Lệnh gọi | Điều kiện | Kết quả |
|---|---|---|
| `BIT_SET` / `BIT_CLR` / `BIT_TGL` | `pos >= 32` | `reg` không đổi |
| `BIT_READ` | `pos >= 32` | `0` |
| `FIELD_GET` | `pos >= 32`, `w > 32`, hoặc `pos + w > 32` | `0` |
| `FIELD_SET` | `pos >= 32`, `w > 32`, hoặc `pos + w > 32` | `reg` không đổi |
| `FIELD_GET` / `FIELD_SET` | `w == 0` | `0` / `reg` không đổi |
| `MASK` | `w >= 32` | `0xFFFFFFFF` |

`pos` âm sẽ bị chuyển thành số không dấu rất lớn nên cũng bị từ chối.

Vì việc từ chối diễn ra im lặng, `FIELD_GET` trả về `0` không cho biết trường thật sự bằng 0 hay tham số sai. Khi tham số là hằng số lúc biên dịch thì lỗi dễ thấy khi review; còn khi tham số được tính lúc chạy, hãy kiểm tra trước khi gọi.

### Ví dụ sử dụng (STM32F103)

Các ví dụ dưới đây dùng bố cục thanh ghi gốc của STM32F103 (LED trên board "Blue Pill" nằm ở PC13).

**Bật clock cho GPIOC** (`RCC->APB2ENR`, bit 4 = `IOPCEN`):

```c
BIT_SET(RCC->APB2ENR, 4);
```

**Cấu hình PC13 làm output push-pull 2 MHz.** `GPIOC->CRH` chứa `CNF13[1:0]` ở bit 23:22 và `MODE13[1:0]` ở bit 21:20. Giá trị reset là `0x44444444`:

```c
FIELD_SET(GPIOC->CRH, 20, 2, 2u);   /* MODE13 = 0b10 (output, 2 MHz) */
FIELD_SET(GPIOC->CRH, 22, 2, 0u);   /* CNF13  = 0b00 (push-pull)     */
/* GPIOC->CRH == 0x44244444 */
```

**Đảo LED và đọc trạng thái chân:**

```c
BIT_TGL(GPIOC->ODR, 13);
if (BIT_READ(GPIOC->IDR, 13)) {
    /* chân đang ở mức cao */
}
```

**Đọc một trường nhiều bit** (ví dụ hệ số nhân PLL trong `RCC->CFGR[21:18]`):

```c
uint32_t pllmul = FIELD_GET(RCC->CFGR, 18, 4);
```

### Ghi chú thiết kế

- **`do { } while (0)` cho các macro dạng câu lệnh.** Macro khai triển thành đúng một câu lệnh, nên `if (x) BIT_SET(r, 1); else BIT_CLR(r, 1);` chạy đúng mà không cần ngoặc nhọn.
- **Dùng `UINT32_C(1)` cho hằng số.** Hằng này có kiểu 32 bit không dấu xác định, không phụ thuộc độ rộng `int` của nền tảng, nên `1 << 31` không bao giờ là trường hợp tràn số có dấu.
- **Kiểm tra biên trước mọi phép shift.** Dịch một giá trị 32 bit đi 32 bit trở lên là hành vi không xác định trong C. Mọi macro đều kiểm tra `pos` và `w` trước, và dùng thêm `(pos) & 31u` làm lớp bảo vệ thứ hai cho chính số bit dịch.
- **`MASK` xử lý riêng `w == 0` và `w >= 32`.** Công thức chung `0xFFFFFFFF >> (32 - w)` sẽ dịch 32 bit khi `w == 0`, nên hai trường hợp này được tách riêng.
- **Mọi tham số của macro đều được đặt trong ngoặc**, nên các biểu thức như `BIT_SET(reg, 2 + 3)` hoạt động đúng.
- **`FIELD_SET` chỉ đọc thanh ghi một lần.** Chu trình read-modify-write gồm một lần đọc và một lần ghi `reg`.

### Hạn chế

- **Chỉ hỗ trợ thanh ghi 32 bit.** Thanh ghi `uint64_t` không được hỗ trợ: ví dụ `BIT_CLR` trên giá trị 64 bit sẽ xóa luôn 32 bit cao. Thanh ghi `uint8_t` và `uint16_t` cũng không nằm trong phạm vi được kiểm thử.
- **Tham số của macro có thể bị đánh giá nhiều lần.** Không truyền biểu thức có tác dụng phụ như `BIT_SET(reg, i++)`.
- **Tham số không hợp lệ thất bại im lặng** (xem bảng ở trên); không có giá trị trả về lỗi hay assertion.
- **`MASK(w)` với `w > 32` trả về mask đầy đủ,** trong khi `FIELD_GET` / `FIELD_SET` coi `w > 32` là không hợp lệ.
- **Tham số `pos` / `w` có dấu có thể gây cảnh báo `-Wsign-compare`** trong phép kiểm tra dải của macro. Hãy truyền giá trị không dấu hoặc hằng số để build không có cảnh báo.
- **`MASK` là tên ngắn và chung chung,** có thể trùng với macro trong header của vendor. Nếu xảy ra xung đột trong project của bạn, hãy đổi tên nó (ví dụ thành `BITOPS_MASK`) trong `bitops.h`.

### Cấu trúc project

```
.
├── include/
│   └── bitops.h          # thư viện (header-only)
├── test/
│   └── test_bitops.c     # unit test
└── README.md
```

### Build và chạy test

Test chạy trên máy host (không cần phần cứng STM32 hay toolchain ARM). Compiler C99 nào cũng dùng được; lệnh dưới đây bật cảnh báo cùng sanitizer cho undefined behaviour và địa chỉ bộ nhớ:

```sh
gcc -std=c99 -Wall -Wextra -Wpedantic -g -fsanitize=undefined,address \
    test/test_bitops.c -o test_bitops
./test_bitops
```

Kết quả mong đợi:

```
===SUMMARY===
Total       : 17831
Test passed : 17831
Test failed : 0
```

Chương trình thoát với mã `0` khi mọi test đều pass và `1` trong trường hợp ngược lại, nên có thể dùng trực tiếp trong CI hoặc target của Makefile. Khi có lỗi, mỗi phép kiểm tra thất bại sẽ in ra file, dòng, biểu thức và, với phép so sánh giá trị, cả giá trị thực tế lẫn giá trị mong đợi.

### Chiến lược kiểm thử

Bộ test chỉ kiểm tra các chức năng công khai của thư viện: `BIT_SET`, `BIT_CLR`, `BIT_TGL`, `BIT_READ`, `MASK`, `FIELD_GET` và `FIELD_SET`.

- **Hằng số tính tay** cho các trường hợp thông thường và trường hợp biên (ví dụ bit 0, bit 31, `0xF78912C4` xóa bit 7 cho ra `0xF7891244`).
- **Quét vét cạn:** mọi vị trí bit từ 0 đến 31, mọi cặp `(pos, w)` hợp lệ và mọi cặp `(pos, w)` không hợp lệ, chạy trên nhiều pattern bit (toàn 0, toàn 1, bit xen kẽ, v.v.).
- **Reference model độc lập.** Giá trị mong đợi được tính bằng toán tử C thông thường và hàm `ref_mask()` dựng từng bit, không dùng chính các macro đang được kiểm tra, nên lỗi trong macro không thể tự triệt tiêu trong phép kiểm tra.
- **Các tính chất:** set/clear là idempotent, toggle hai lần trả về giá trị ban đầu, `FIELD_SET` rồi `FIELD_GET` trả về `val & MASK(w)`, bit ngoài trường không bị đổi, `BIT_READ` và `FIELD_GET` không làm thay đổi thanh ghi.
- **Đầu vào không hợp lệ:** `pos` bằng 32, 33, 100, `0xFFFFFFFF` và `-1`; độ rộng từ 33 trở lên; trường vượt qua bit 31.
- **Các dạng tham số:** hằng số lúc biên dịch, biến lúc chạy và biểu thức số học.

Các test chạy trên host nên chỉ kiểm tra logic của macro. Chúng không đo kích thước mã sinh ra hay thời gian thực thi trên Cortex-M3.