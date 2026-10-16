#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "ieee754_fpu.h"

uint32_t float_to_bits(float f) {
    uint32_t bits;
    memcpy(&bits, &f, sizeof(bits));
    return bits;
}

float bits_to_float(uint32_t bits) {
    float f;
    memcpy(&f, &bits, sizeof(f));
    return f;
}

void assert_fpu_op(const char* op_name, float a, float b, uint32_t sim_result, float host_result) {
    uint32_t expected_bits = float_to_bits(host_result);

    printf("[ %s ] a = %e, b = %e\n", op_name, a, b);
    printf("  Sim FPU : 0x%08X (%e)\n", sim_result, bits_to_float(sim_result));
    printf("  Host FPU: 0x%08X (%e)\n", expected_bits, host_result);

    if (sim_result == expected_bits || (isnan(bits_to_float(sim_result)) && isnan(host_result))) {
        printf("  => ESTADO: EXITO (Match)\n\n");
    } else {
        printf("  => ESTADO: FALLO (Mismatch arquitectonico)\n\n");
    }
}

int main() {
    printf("=== INICIANDO VALIDACION DE FPU IEEE 754 ===\n\n");

    fpu_rmode = RNE;
    fpu_flags = 0;

    float a = 3.141592f;
    float b = 2.718281f;

    assert_fpu_op("SUMA", a, b, fpu_add(float_to_bits(a), float_to_bits(b)), a + b);
    assert_fpu_op("RESTA", a, b, fpu_sub(float_to_bits(a), float_to_bits(b)), a - b);
    assert_fpu_op("MULTIPLICACION", a, b, fpu_mul(float_to_bits(a), float_to_bits(b)), a * b);
    assert_fpu_op("DIVISION", a, b, fpu_div(float_to_bits(a), float_to_bits(b)), a / b);


    float zero = 0.0f;
    float inf = INFINITY;
    float neg_inf = -INFINITY;

    printf("=== VALIDACION DE CASOS EXTREMOS ===\n\n");
    assert_fpu_op("DIV POR CERO", a, zero, fpu_div(float_to_bits(a), float_to_bits(zero)), a / zero);
    assert_fpu_op("INF + INF", inf, inf, fpu_add(float_to_bits(inf), float_to_bits(inf)), inf + inf);
    assert_fpu_op("INF - INF", inf, inf, fpu_sub(float_to_bits(inf), float_to_bits(inf)), inf - inf);

    printf("Estado final de los Flags del Sistema (Hex): 0x%02X\n", fpu_flags);
    assert_fpu_op("INF + (-INF)", inf, neg_inf, fpu_add(float_to_bits(inf), float_to_bits(neg_inf)), inf + neg_inf);
    assert_fpu_op("-INF - (-INF)", neg_inf, neg_inf, fpu_sub(float_to_bits(neg_inf), float_to_bits(neg_inf)), neg_inf - neg_inf);
    assert_fpu_op("-INF + NUM", neg_inf, a, fpu_add(float_to_bits(neg_inf), float_to_bits(a)), neg_inf + a);
    if (fpu_flags & FLAG_NV) printf("- Operacion Invalida (NaN) detectada.\n");

    return 0;
}