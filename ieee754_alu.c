#include "ieee754_fpu.h"

static uint32_t count_leading_zeros(uint32_t x) {
    if (x == 0) return 32;
    uint32_t n = 0;
    if ((x & 0xFFFF0000) == 0) { n += 16; x <<= 16; }
    if ((x & 0xFF000000) == 0) { n +=  8; x <<=  8; }
    if ((x & 0xF0000000) == 0) { n +=  4; x <<=  4; }
    if ((x & 0xC0000000) == 0) { n +=  2; x <<=  2; }
    if ((x & 0x80000000) == 0) { n +=  1; }
    return n;
}

// Algoritmo de Suma Unificada (Suma y Resta)
static uint32_t fpu_add_sub(uint32_t a, uint32_t b, bool subtract) {
    raw_float ra = unpack_float(a);
    raw_float rb = unpack_float(b);

    // INVERSIÓN DE SIGNO ÚNICA PARA LA RESTA
    if (subtract) {
        rb.sign ^= 1;
    }

    // CIRCUITO DE EXCEPCIONES PARA INFINITOS
    if (ra.is_inf || rb.is_inf) {
        if (ra.is_inf && rb.is_inf) {
            if (ra.sign != rb.sign) {
                // Infinito - Infinito (o Inf + (-Inf)) es una operación inválida
                raw_float nan_res;
                nan_res.is_nan = true;
                nan_res.sign = 1; // qNaN negativo (Coincidencia con FPU Host)
                return pack_and_round(nan_res);
            }
            return pack_and_round(ra); // Mismo signo, prevalece el infinito
        }
        return pack_and_round(ra.is_inf ? ra : rb);
    }

    // EXCEPCIONES PARA NAN
    if (ra.is_nan || rb.is_nan) {
        ra.is_nan = true;
        return pack_and_round(ra);
    }

    // ALINEACIÓN DE EXPONENTES (Shifter)
    if (ra.exp < rb.exp) {
        raw_float temp = ra; ra = rb; rb = temp;
    }

    int32_t exp_diff = ra.exp - rb.exp;

    // DESPLAZAMIENTO Y BIT STICKY
    if (exp_diff > 0) {
        if (exp_diff >= 27) {
            rb.mant = 1;
        } else {
            uint32_t sticky_mask = (1 << exp_diff) - 1;
            uint32_t sticky_bit = (rb.mant & sticky_mask) ? 1 : 0;
            rb.mant = (rb.mant >> exp_diff) | sticky_bit;
        }
    }

    raw_float res;
    res.exp = ra.exp;
    res.is_zero = false; res.is_inf = false; res.is_nan = false;

    // ALU DE MANTISAS
    if (ra.sign == rb.sign) {
        res.sign = ra.sign;
        res.mant = ra.mant + rb.mant;
    } else {
        if (ra.mant >= rb.mant) {
            res.sign = ra.sign;
            res.mant = ra.mant - rb.mant;
        } else {
            res.sign = rb.sign;
            res.mant = rb.mant - ra.mant;
        }

        // NORMALIZACIÓN POR CANCELACIÓN MASIVA
        if (res.mant == 0) {
            res.is_zero = true;
            res.sign = (fpu_rmode == RMI) ? 1 : 0;
        } else {
            uint32_t clz = count_leading_zeros(res.mant);
            uint32_t shift_needed = clz - (32 - 27);

            if (shift_needed > 0) {
                res.mant <<= shift_needed;
                res.exp -= shift_needed;
            }
        }
    }

    return pack_and_round(res);
}

uint32_t fpu_add(uint32_t a, uint32_t b) { return fpu_add_sub(a, b, false); }
uint32_t fpu_sub(uint32_t a, uint32_t b) { return fpu_add_sub(a, b, true);  }

// Multiplicación (Simulando un árbol Wallace 24x24)
uint32_t fpu_mul(uint32_t a, uint32_t b) {
    raw_float ra = unpack_float(a);
    raw_float rb = unpack_float(b);
    raw_float res;

    res.is_zero = ra.is_zero || rb.is_zero;
    res.is_inf = ra.is_inf || rb.is_inf;
    res.is_nan = ra.is_nan || rb.is_nan;

    if ((ra.is_zero && rb.is_inf) || (ra.is_inf && rb.is_zero)) {
        res.is_nan = true; return pack_and_round(res);
    }

    res.sign = ra.sign ^ rb.sign;
    res.exp = ra.exp + rb.exp;

    uint64_t prod = (uint64_t)(ra.mant >> 3) * (uint64_t)(rb.mant >> 3);

    // Ajuste rígido para alinear el '1' implícito al bit 26
    uint32_t shift_amount = (prod & (1ULL << 47)) ? 21 : 20;

    res.mant = (uint32_t)(prod >> shift_amount);

    if (prod & ((1ULL << shift_amount) - 1)) res.mant |= 1;

    if (shift_amount == 21) res.exp++;

    return pack_and_round(res);
}

// División Iterativa
uint32_t fpu_div(uint32_t a, uint32_t b) {
    raw_float ra = unpack_float(a);
    raw_float rb = unpack_float(b);
    raw_float res;

    res.sign = ra.sign ^ rb.sign;
    res.is_zero = false; res.is_inf = false; res.is_nan = false;

    if (rb.is_zero) {
        if (ra.is_zero) res.is_nan = true;
        else { res.is_inf = true; fpu_flags |= FLAG_DZ; }
        return pack_and_round(res);
    }
    if (ra.is_zero) { res.is_zero = true; return pack_and_round(res); }

    res.exp = ra.exp - rb.exp;

    uint64_t num = (uint64_t)(ra.mant >> 3) << 26;
    uint32_t den = rb.mant >> 3;

    uint32_t quotient = (uint32_t)(num / den);
    uint32_t remainder = (uint32_t)(num % den);

    if (quotient & (1 << 26)) {
        res.mant = quotient;
    } else {
        res.mant = quotient << 1;
        res.exp--;
    }

    if (remainder != 0) res.mant |= 1;

    return pack_and_round(res);
}