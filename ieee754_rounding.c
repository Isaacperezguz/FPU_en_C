#include "ieee754_fpu.h"

rounding_mode_t fpu_rmode = RNE;
uint8_t fpu_flags = 0;

// Simula el decodificador de entrada de la FPU
raw_float unpack_float(uint32_t f) {
    raw_float r;
    r.sign = (f >> 31) & 1;
    uint32_t raw_exp = (f >> 23) & 0xFF;
    uint32_t raw_mant = f & 0x7FFFFF;

    r.is_zero = false; r.is_inf = false; r.is_nan = false;

    if (raw_exp == 0xFF) {
        if (raw_mant == 0) r.is_inf = true;
        else r.is_nan = true;
        r.exp = 255;
        r.mant = raw_mant << 3; // Mantener payload del NaN
    } else if (raw_exp == 0) {
        if (raw_mant == 0) {
            r.is_zero = true;
            r.exp = 0;
            r.mant = 0;
        } else {
            // Número Subnormal
            r.exp = -126;
            r.mant = raw_mant << 3; // No hay 1 implícito
        }
    } else {
        // Número Normal
        r.exp = raw_exp - 127;
        r.mant = (raw_mant | 0x800000) << 3; // Añadir 1 implícito y dejar espacio GRS
    }
    return r;
}

// Simula el normalizador, la lógica de redondeo y el codificador de salida
uint32_t pack_and_round(raw_float r) {
    if (r.is_nan) {
        fpu_flags |= FLAG_NV;
        return (r.sign << 31) | 0x7FC00000;
    }
    if (r.is_inf) return (r.sign << 31) | 0x7F800000;
    if (r.is_zero) return (r.sign << 31);

    if (r.mant & (1 << 27)) {
        r.mant = (r.mant >> 1) | (r.mant & 1); // El bit que sale va al Sticky
        r.exp++;
    }

    int32_t final_exp = r.exp + 127;

    if (final_exp <= 0) {

        int32_t shift_amount = 1 - final_exp;

        if (shift_amount >= 27) {
            r.mant = (r.mant != 0) ? 1 : 0; // Todo colapsa al sticky bit
        } else {
            uint32_t sticky_mask = (1 << shift_amount) - 1;
            uint32_t sticky_bit = (r.mant & sticky_mask) ? 1 : 0;
            r.mant = (r.mant >> shift_amount) | sticky_bit;
        }
        final_exp = 0; // Exponente oficial para números subnormales
        fpu_flags |= FLAG_UF; // Levantamos bandera de Underflow
    }

    uint8_t G = (r.mant >> 2) & 1;
    uint8_t R = (r.mant >> 1) & 1;
    uint8_t S = r.mant & 1;
    bool inexact = (G | R | S) != 0;

    if (inexact) fpu_flags |= FLAG_NX;

    bool round_up = false;
    switch (fpu_rmode) {
        case RNE:
            if (G && (R || S || ((r.mant >> 3) & 1))) round_up = true;
            break;
        case RTZ:
            round_up = false;
            break;
        case RPI:
            if (!r.sign && inexact) round_up = true;
            break;
        case RMI:
            if (r.sign && inexact) round_up = true;
            break;
    }

    uint32_t final_mant = r.mant >> 3; // Quitar G, R, S

    if (round_up) {
        final_mant++;
        if (final_mant & 0x1000000) { // El redondeo causó un desbordamiento en la mantisa
            final_mant >>= 1;
            final_exp++;
        }
    }

    //Chequeo de Overflow (Infinito)
    if (final_exp >= 255) {
        fpu_flags |= FLAG_OF | FLAG_NX;
        return (r.sign << 31) | 0x7F800000;
    }

    //Empaquetado en los cables físicos de 32 bits
    return (r.sign << 31) | (final_exp << 23) | (final_mant & 0x7FFFFF);
}