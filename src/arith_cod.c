#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "arith_cod.h"

static void set_bit_value(unsigned char* out, int index, int bit_value) {
    if (bit_value) {
        out[index / 8] |= (1 << (7 - (index % 8)));
    } else {
        out[index / 8] &= ~(1 << (7 - (index % 8)));
    }
}

static int get_bit_value(const unsigned char* buf, int index) {
    return (buf[index / 8] >> (7 - (index % 8))) & 1;
}

static void propagate_carry(unsigned char* out, ac_state_t* state) {
    int idx = state->out_index - 1;
    while (idx >= 0 && get_bit_value(out, idx) == 1) {
        set_bit_value(out, idx, 0);
        idx--;
    }
    if (idx >= 0) set_bit_value(out, idx, 1);
}

void init_state(ac_state_t* state, int precision) {
    state->prob_table  = (int*)malloc(sizeof(int) * 256);
    state->cumul_table = (int*)malloc(sizeof(int) * 257);
    state->frac_size   = precision;
    state->one_counter = 0;
    state->last_symbol = -1;
    state->current_symbol = 0;
    state->current_index  = 0;
    state->out_index = 0;
    state->base   = 0;
    state->length = (1 << precision) - 1;

    assert(state->prob_table && state->cumul_table && "memory allocation failed");
}

void build_probability_table(ac_state_t* state, const unsigned char* in, int size) {
    for(int i = 0; i < 256; i++) state->prob_table[i] = 1;
    for(int i = 0; i < size; i++) state->prob_table[in[i]]++;
    state->cumul_table[0] = 0;
    for(int i = 0; i < 256; i++) {
        state->cumul_table[i+1] = state->cumul_table[i] + state->prob_table[i];
    }
}

static int state_half_length(const ac_state_t* state) {
    return 1 << (state->frac_size - 1);
}

static int modulo_precision(const ac_state_t* state, int value) {
    return value & ((1 << state->frac_size) - 1);
}

static void output_digit(unsigned char* out, ac_state_t* state, int digit) {
    set_bit_value(out, state->out_index, digit);
    state->out_index++;
}

static void normalize(unsigned char* out, ac_state_t* state) {
    while (state->length < state_half_length(state)) {
        int digit = (state->base >> (state->frac_size - 1)) & 1;
        output_digit(out, state, digit);
        state->base = (state->base << 1) & ((1 << state->frac_size) - 1);
        state->length <<= 1;
        state->length = modulo_precision(state, state->length);
    }
}

static void encode_symbol(unsigned char* out, ac_state_t* state, unsigned char symbol) {
    int total = state->cumul_table[256];
    int range = state->length + 1;
    int low_count = state->cumul_table[symbol];
    int high_count = state->cumul_table[symbol+1];
    int left  = (int)(( (long long)range * low_count ) / total);
    int right = (int)(( (long long)range * high_count ) / total) - 1;

    state->base  = modulo_precision(state, state->base + left);
    state->length = right - left + 1;
    if (state->base + right < state->base) propagate_carry(out, state);
    normalize(out, state);
}

void encode_value(unsigned char* out, const unsigned char* in, size_t size, ac_state_t* state) {
    for(size_t i = 0; i < size; i++) {
        encode_symbol(out, state, in[i]);
    }
    // 最後將整個 interval 壓縮到最小
    for(int i = 0; i < state->frac_size; i++) {
        int digit = (state->base >> (state->frac_size - 1)) & 1;
        output_digit(out, state, digit);
        state->base = (state->base << 1) & ((1 << state->frac_size) - 1);
    }
}

static void init_decoding(ac_state_t* state, const unsigned char* in) {
    state->base   = 0;
    state->out_index = 0;
    for(int i = 0; i < state->frac_size; i++) {
        state->base = (state->base << 1) | get_bit_value(in, i);
        state->out_index++;
    }
}

static unsigned char decode_symbol(unsigned char* in, ac_state_t* state) {
    int total = state->cumul_table[256];
    int range = state->length + 1;
    int scaled_value = (int)(( (long long)(state->base) * total ) / range);
    int symbol;
    for(symbol = 255; symbol >= 0; symbol--) {
        if(state->cumul_table[symbol] <= scaled_value) break;
    }
    int low_count  = state->cumul_table[symbol];
    int high_count = state->cumul_table[symbol+1];

    int left  = (int)(( (long long)range * low_count ) / total);
    int right = (int)(( (long long)range * high_count ) / total) - 1;

    state->base   = modulo_precision(state, state->base - left);
    state->length = right - left + 1;
    if ((int)state->base > right) propagate_carry(in, state);
    while (state->length < state_half_length(state)) {
        state->base   = ( (state->base << 1) | get_bit_value(in, state->out_index) ) & ((1 << state->frac_size) - 1);
        state->out_index++;
        state->length <<= 1;
        state->length = modulo_precision(state, state->length);
    }
    return (unsigned char)symbol;
}

void decode_value(unsigned char* out, unsigned char* in, ac_state_t* state, size_t expected_size) {
    init_decoding(state, in);

    for(size_t i = 0; i < expected_size; i++) {
        printf("Decoding %ld/%ld\n", i, expected_size);
        out[i] = decode_symbol(in, state);
        printf("Decoded symbol: %d\n", out[i]);
    }
}