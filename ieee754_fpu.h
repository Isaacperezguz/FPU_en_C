#ifndef IEEE754_FPU_H
#define IEEE754_FPU_H

#include <stdint.h>
#include <stdbool.h>

// Modos de redondeo del estándar IEEE 754
typedef enum {
    RNE = 0, // Round to Nearest, ties to Even (Default)
    RTZ = 1, // Round Toward Zero (Truncate)
    RPI = 2, // Round Toward Plus Infinity
    RMI = 3  // Round Toward Minus Infinity
} rounding_mode_t;

extern rounding_mode_t fpu_rmode;
extern uint8_t fpu_flags;

#define FLAG_NX 0x01 // Inexact (Inexacto / Redondeo aplicado)
#define FLAG_UF 0x02 // Underflow (Subdesbordamiento)
#define FLAG_OF 0x04 // Overflow (Sobredesbordamiento)
#define FLAG_DZ 0x08 // Division by Zero
#define FLAG_NV 0x10 // Invalid Operation (Ej. 0/0, Inf-Inf)

typedef struct {
    uint8_t sign;
    int32_t exp;
    uint32_t mant;
    bool is_zero;
    bool is_inf;
    bool is_nan;
} raw_float;

// Prototipos de operaciones
uint32_t fpu_add(uint32_t a, uint32_t b);
uint32_t fpu_sub(uint32_t a, uint32_t b);
uint32_t fpu_mul(uint32_t a, uint32_t b);
uint32_t fpu_div(uint32_t a, uint32_t b);

// Prototipos internos
raw_float unpack_float(uint32_t f);
uint32_t pack_and_round(raw_float r);

#endif // IEEE754_FPU_H