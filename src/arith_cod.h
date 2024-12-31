#pragma once


/** Arithmetic Coding state structure */
typedef struct
{
    int* prob_table;
    int* cumul_table;
    int  frac_size;
    int one_counter;
    int current_index;
    int out_index;
    int last_symbol;
    unsigned char current_symbol;
    int base;
    int length;

} ac_state_t;

void init_state(ac_state_t* state, int precision);

void build_probability_table(ac_state_t* state, const unsigned char* in, int size);

void reset_uniform_probability(ac_state_t* state);

void transform_count_to_cumul(ac_state_t* state, int size);

void reset_prob_table(ac_state_t* state);

void display_prob_table(ac_state_t* state);

void encode_value(unsigned char* out, const unsigned char* in,size_t size,
            ac_state_t* state);

void encode_character(unsigned char* out, unsigned char in, ac_state_t* state);

void select_value(unsigned char* out, ac_state_t* state);

void init_decoding(unsigned char* in, ac_state_t* state);

unsigned char decode_character( unsigned char* in, ac_state_t* state);

void decode_value(unsigned char* out, unsigned char* in, ac_state_t* state,
        size_t expected_size);

void encode_value_with_update(unsigned char* out, unsigned char* in,
                    size_t size, ac_state_t* state, int update_range,
                    int range_clear);
                    
void decode_value_with_update(unsigned char* out, unsigned char* in,
                    ac_state_t* state, size_t expected_size,
                    int update_range, int range_clear);

