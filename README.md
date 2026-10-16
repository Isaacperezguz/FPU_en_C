## Overview
This project is a C-based, bit-level simulator of a Single-Precision (32-bit) IEEE 754 Floating-Point Unit (FPU). It is designed to model the low-level hardware execution of arithmetic operations, bypassing the host CPU's native floating-point instructions. 

Crucially, this project includes a **Hardware Validation Suite** (`main.c`) that cross-references the simulated bitwise outputs against the physical host CPU to ensure architectural match and strict compliance with the IEEE 754 standard.

## Core Features
*   **ALU Operations:** Implements Unified Addition/Subtraction, Wallace-tree simulated Multiplication, and Iterative Division at the bitwise level.
*   **Hardware-Level Emulation:** Uses custom packing/unpacking algorithms to extract the sign, exponent, and mantissa into a `raw_float` struct.
*   **Rounding Modes:** Supports standard IEEE 754 rounding hardware logic (RNE, RTZ, RPI, RMI).
*   **Exception Handling:** Rigorously detects and manages edge cases including `NaN`, Infinity, Subnormals, Division by Zero, Overflow, and Underflow flags.
*   **Automated Validation Bench:** Uses assertion testing to validate the FPU state and flag generation against native compiler math execution.

## Architecture & Modules
*   `ieee754_alu.c`: Core arithmetic logic unit implementing bitwise shifts, sticky bits calculation, and mantissa alignment.
*   `ieee754_rounding.c`: Input decoder (`unpack_float`) and output encoder/normalizer (`pack_and_round`).
*   `ieee754_fpu.h`: System definitions, rounding mode enums, and hardware exception flag macros (`FLAG_NX`, `FLAG_UF`, `FLAG_OF`, `FLAG_DZ`, `FLAG_NV`).
*   `main.c`: The automated validation testbench.

## Compilation and Execution
This project uses standard C libraries and requires a standard GCC/Clang compiler.

```bash
# Compile the FPU simulator and validation suite
gcc main.c ieee754_alu.c ieee754_rounding.c -o fpu_validator -lm

# Run the validation testbench
./fpu_validator
```
## Validation Output Example

+ The testbench outputs a direct comparison between the simulated bit-string and the host CPU output, raising a Match or Mismatch state:

=== INICIANDO VALIDACION DE FPU IEEE 754 ===

[ SUMA ] a = 3.141592e+00, b = 2.718281e+00
  Sim FPU : 0x40BB85F8 (5.859873e+00)
  Host FPU: 0x40BB85F8 (5.859873e+00)
  => ESTADO: EXITO (Match)
