#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "arith_cod.h"


void init_state(ac_state_t* state, int precision) 
{
    state->prob_table = calloc(sizeof(int) * 256,1);
    state->cumul_table = calloc(sizeof(int) * 257,1);

    state->frac_size = precision;

    state->one_counter = 0;
    state->last_symbol = -1;

    state->current_symbol = 0;
    state->current_index  = 0;

    state->out_index = 0;

    state->base = 0;
    state->length = (1 << precision) - 1;

    assert(state->prob_table && state->cumul_table && "memory allocation failed");
}

void transform_count_to_cumul(ac_state_t* state, int _)  
{
    (void)_; // unused
    int i;
    int size = 0;
    int alphabet_size = 256;
    for (i = 0; i < alphabet_size; ++i) size += state->prob_table[i];

    for (i = 0; i < alphabet_size; ++i) {
        int count = state->prob_table[i];
        int local_prob = ((long long) count * ((1 << state->frac_size)-258)) / (size);
        //if (i == 0) state->cumul_table[0] = state->prob_table[0];
        //else state->cumul_table[i] = state->cumul_table[i-1] + state->prob_table[i];
        if (i == 0) {
            state->cumul_table[0] = 0;
        }
            state->cumul_table[i+1] = state->cumul_table[i] + local_prob;
    }
    state->cumul_table[256] = ((long long) 1 << state->frac_size) - 1;
}

void build_probability_table(ac_state_t* state, const unsigned char* in, int size) 
{
    int alphabet_size = 256;
    int count_weight = 1;
    int i;
    // reset 
    for (i = 0; i < alphabet_size; ++i) state->prob_table[i] = 1;

    // occurences counting
    for (i = 0; i < size; ++i) state->prob_table[in[i]] += count_weight;

    // normalization according to state format
    transform_count_to_cumul(state, count_weight * size);

}

void reset_uniform_probability(ac_state_t* state)
{
    int alphabet_size = 256;
    int size = 0;
    int i;

    for (i = 0; i < 256; ++i) state->prob_table[i] = 1;

    for (i = 0; i < 256; ++i) {
        int count = state->prob_table[i];
        state->prob_table[i] = ((long long) count * (1 << state->frac_size)) / (size + alphabet_size);
        if (i == 0) {
            state->cumul_table[0] = 0;
            state->cumul_table[1] = state->prob_table[0];
        }
        else state->cumul_table[i+1] = state->cumul_table[i] + state->prob_table[i];
    }
}

void reset_prob_table(ac_state_t* state)
{
    int i;
    for (i = 0; i < 256; ++i) state->prob_table[i] = 1;
}

void display_prob_table(ac_state_t* state) 
{
    int i;
    double norm = (double) ((1 << state->frac_size));
    for (i = 0; i < 256; i++) {
        printf("P[%i]=%.6f, C[%i/%x]=%.6f / %x\n", i, state->prob_table[i] / norm, i, i, state->cumul_table[i] / norm, state->cumul_table[i]); 
    }
    i = 256;
    printf("P[%i]=%.6f, C[%i/%02x]=%.6f / %x\n", i, 0.0, i, i, state->cumul_table[i] / norm, state->cumul_table[i]); 
}

void set_bit_value(unsigned char* out, int index, int bit_value) 
{
    if (bit_value) {
        out[index / 8] |= (1 << (7 - (index % 8)));
    } else {
        out[index / 8] &= ~(1 << ((7 - index % 8)));
    }
}


int get_bit_value(unsigned char* out, int index) 
{
    return (out[index / 8] >> (7 - (index % 8))) & 0x1;
}


unsigned char* output_zero(unsigned char* out, ac_state_t* state) 
{
    assert(state->out_index >= 0 && "out_index must be positive");

    set_bit_value(out, state->out_index, 0);
    state->out_index++;

    return NULL;
}


unsigned char* output_one(unsigned char* out, ac_state_t* state) 
{
    assert(state->out_index >= 0 && "out_index must be positive");

    set_bit_value(out, state->out_index, 1);
    state->out_index++;

    return NULL;
}


void output_digit(unsigned char* out, ac_state_t* state, int digit)
{
    switch (digit) {
    case 0:
        output_zero(out, state);
        break;
    case 1:
        output_one(out, state);
        break;
    default:
        assert(0 && "unexpected digit value in output_digit\n");
        break;
    }
}


void display_bin(unsigned char* out, int bit_size) 
{
        (void)out; 
        (void)bit_size;
    #ifdef DEBUG
    int i;
    DEBUG_PRINTF("bin_value=0.");
    for (i = 0; i < bit_size; ++i) DEBUG_PRINTF("%01d", get_bit_value(out, i));
    DEBUG_PRINTF("\n");
    #endif
}


void propagate_carry(unsigned char* out, ac_state_t* state) 
{
    int index = state->out_index - 1;
    while (get_bit_value(out, index) == 1) {
        set_bit_value(out, index, 0);
        index--;
    }
    set_bit_value(out, index, 1);
}


int state_half_length(ac_state_t* state)
{
    return 1 << (state->frac_size - 1);
}


int modulo_precision(ac_state_t* state, int value) 
{
    return value % (1 << state->frac_size);
}

void encode_character(unsigned char* out, unsigned char in, ac_state_t* state) 
{
    int in_cumul   = state->cumul_table[in];

    // interval update
    int Y = ((long long) state->length * state->cumul_table[in + 1]) >> state->frac_size;
    int base_increment = ((long long) state->length * in_cumul) >> state->frac_size;

    int new_base   = modulo_precision(state, state->base + base_increment);
    int new_length = Y - base_increment;

    assert(new_base >= 0 && new_length > 0 && "intermediary values must be positive");

    if (new_base < state->base) {
        // propagate carry
        propagate_carry(out, state);
    }


    while (new_length < state_half_length(state)) {
        // renormalization
        int digit = (new_base * 2) >> state->frac_size;
        output_digit(out, state, digit);
        new_length = modulo_precision(state, 2 * new_length);
        new_base   = modulo_precision(state, 2 * new_base);
    }

    state->base   = new_base;
    state->length = new_length;

}

unsigned char decode_character( unsigned char* in, ac_state_t* state) 
{
    // input value
    int length = state->length;
    int V      = state->base;
    int t      = state->out_index;

    //printf(state->cumul_table[256]);

    // interval selection
    //printf("test1");
    int s = 0, n = 256, X = 0;
    long long Y= ((long long) length * state->cumul_table[256]) >> state->frac_size;
    // 使用 Y 進行後續操作
    //printf("cumul %d",state->cumul_table[256]);
    //printf("Y: %lld\n", Y);
    //printf("test2");

        
        while (n - s > 1) {
        int m = (s + n) / 2;
        int Z = ((long long) length * state->cumul_table[m]) >> state->frac_size;

        if (Z > V) { n = m; Y = Z;}
        else { s = m; X = Z;};
        }
        
    //printf("test3");


    V = V - X;
    length = Y - X;

    
    while (length < state_half_length(state)) {
        // renormalization
        t++;
        V = modulo_precision(state, 2 * V) + get_bit_value(in, t);
        length = modulo_precision(state, 2 * length);
    }
    

    //output
    state->length    = length;
    state->base      = V;
    state->out_index = t;
    
    

    return s;
}

void select_value(unsigned char* out, ac_state_t* state) 
{
    // code value selection (flushing buffer)
    int base = state->base;
    int new_base = modulo_precision(state, state->base + state_half_length(state) / 2);
    int new_length = (1 << (state->frac_size - 2)) - 1;
    if (base > new_base) propagate_carry(out, state);

    // renormalization (output two symbols)
    while (new_length < state_half_length(state)) {
        int digit = (new_base * 2) >> state->frac_size;
        output_digit(out, state, digit);
        new_length = modulo_precision(state, 2 * new_length);
        new_base   = modulo_precision(state, 2 * new_base);
    }
}

void encode_value(unsigned char* out, const unsigned char* in, size_t size, ac_state_t* state) 
{
    int i=0;
    size_t j;
    
    // encoding each character
    for (j = 0; j < size; ++j){
        encode_character(out, in[i], state);
        i++;
    }

    select_value(out, state);
}

void init_decoding(unsigned char* in, ac_state_t* state)
{
    int length = (1 << state->frac_size) - 1;
    int V = 0;
    int k;
    for (k = 0; k < state->frac_size; k++) {
        V |= get_bit_value(in, k) << (state->frac_size - 1 - k);
    }

    int t = state->frac_size - 1;

    state->out_index = t;
    state->base      = V;
    state->length    = length;
}

void decode_value(unsigned char* out, unsigned char* in, ac_state_t* state, size_t expected_size) 
{

    init_decoding(in, state);
    size_t i;
    
    for (i = 0; i < expected_size; ++i) {
        
        *(out++) = decode_character(in, state);

    }
    

}

void encode_value_with_update(unsigned char* out, unsigned char* in, size_t size, ac_state_t* state, int update_range, int range_clear) 
{

    int update_count = 0;
    int k;
    // reseting count
    for (size_t i = 0; i < 256; i++){
        state->prob_table[k] = 1;
        k++;
    }


    k=0;
    // encoding each character
    for (size_t i = 0; i < size; ++i) {
        unsigned char input_char = in[k];
        k++;
        encode_character(out, input_char, state);
        // updating prob
        state->prob_table[input_char]++;
        update_count++;

        // updating cumul table
        if (update_count >= update_range) {
            transform_count_to_cumul(state, update_count);
            // reseting count
            if (range_clear) {
                update_count = 0;
                int j;
                for (j = 0; j < 256; j++) state->prob_table[j] = 1;
            }
        }
    }

    // code value selection (flushing buffer)

    int base = state->base;
    int new_base = modulo_precision(state, state->base + state_half_length(state) / 2);
    int new_length = (1 << (state->frac_size - 2)) - 1;
    if (base > new_base) propagate_carry(out, state);

    // renormalization (output two symbols)
    while (new_length < state_half_length(state)) {
        int digit = (new_base * 2) >> state->frac_size;
        output_digit(out, state, digit);
        new_length = modulo_precision(state, 2 * new_length);
        new_base   = modulo_precision(state, 2 * new_base);
    }
}

void decode_value_with_update(unsigned char* out, unsigned char* in, ac_state_t* state, size_t expected_size, int update_range, int range_clear) 
{
    int update_count = 0;
    int length = (1 << state->frac_size) - 1;
    int V = 0;
    int k;
    for (k = 0; k < state->frac_size; k++) {
        V |= get_bit_value(in, k) << (state->frac_size - 1 - k);
    }
    // int V = (in[0] << 8) | in[1];

    // reseting count
    
    int temp=0;
    for (size_t i = 0; i < 256; i++){
        state->prob_table[temp] = 1;
        temp++;
    }

    int t = state->frac_size - 1;

    state->out_index = t;
    state->base      = V;
    state->length    = length;

    for (size_t i = 0; i < expected_size; ++i) {
        unsigned char decoded_char = decode_character(in, state);
        *(out++) = decoded_char; 

        // updating prob
        state->prob_table[decoded_char]++;
        update_count++;

        // updating cumul table
        if (update_count >= update_range) {
            transform_count_to_cumul(state, update_count);
            // reseting count
            if (range_clear) {
                update_count = 0;
                int j;
                for (j = 0; j < 256; j++) state->prob_table[j] = 1;
            };
        }

    }

}

/*#ifndef DEBUG
#define DEBUG_PRINTF(...)
#define DISPLAY_VALUE

#else
#define DEBUG_PRINTF(...) printf(__VAARGS__)

#define DISPLAY_VALUE(name, state, value) {\
  printf(name "= %.6f\n", (value) / (double) (1 << (state)->frac_size));\
}
#endif
*/