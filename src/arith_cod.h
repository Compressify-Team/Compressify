#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef ARITH_CODING_H
#define ARITH_CODING_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int *prob_table;
    int *cumul_table;
    int frac_size;
    int one_counter;
    int last_symbol;
    int current_symbol;
    int current_index;
    int out_index;
    int base;
    int length;
} ac_state_t;

void init_state(ac_state_t* state, int precision);
void build_probability_table(ac_state_t* state, const unsigned char* in, int size);
void encode_value(unsigned char* out, const unsigned char* in, size_t size, ac_state_t* state);
void decode_value(unsigned char* out, unsigned char* in, ac_state_t* state, size_t expected_size);


#ifdef __cplusplus
}
#endif

#endif