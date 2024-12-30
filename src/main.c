#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "arith_cod.h"

// Function declarations for compression algorithms
void huffman_compress(const char *file_content, size_t file_size, const char *input_file) 
{
    char output_file[256];
    snprintf(output_file, sizeof(output_file), "%s.huf", input_file);
    printf("Compressing %s to %s using Huffman Coding...\n", input_file, output_file);

    // Open the output file for writing
    FILE *out_file = fopen(output_file, "wb");
    if (out_file == NULL) {
        perror("Error opening output file");
        return;
    }

    // TODO: Add Huffman Coding implementation here
    fprintf(out_file, "Example compressed content (Huffman)\n");

    // Close the output file
    fclose(out_file);
}

size_t calculate_output_size(const unsigned char *output, size_t max_size) {
    size_t size = 0;
    while (size < max_size && output[size] != '\0') {
        size++;
    }
    return size;
}

void arithmetic_compress(const char *file_content, size_t file_size, const char *input_file) {
    size_t input_size = strlen(file_content);

    size_t output_size = input_size * 2; // 預估輸出大小

    unsigned char* input = (unsigned char*)file_content;
    unsigned char* output = calloc(sizeof(unsigned char) * output_size, 1);

    // 初始化編碼狀態
    ac_state_t encoder_state;
    init_state(&encoder_state, 16);

    // 建立機率表
    build_probability_table(&encoder_state, input, input_size);

    // 編碼
    printf("Encoding...\n");

    encode_value(output, input, input_size, &encoder_state);

    // 將結果寫入 .arc 檔案
    char output_file[256];

    snprintf(output_file, sizeof(output_file), "%s.arc", input_file);
    FILE *out_file = fopen(output_file, "wb");
    if (out_file == NULL) {
        perror("Error opening output file");
        free(output);
        return;
    }
    
    // 計算實際輸出大小
    size_t actual_output_size = calculate_output_size(output, output_size);

    fwrite(&input_size, sizeof(size_t), 1, out_file);
    printf("input_size: %zu\n", input_size);

    //fwrite(&encoder_state, sizeof(ac_state_t), 1, out_file);
    // 單獨寫入 encoder_state 的每個成員

    fwrite(encoder_state.prob_table, sizeof(int), 256, out_file); // 假設 prob_table 大小為 256
    fwrite(encoder_state.cumul_table, sizeof(int), 257, out_file); // 假設 cumul_table 大小為 256
    fwrite(&encoder_state.frac_size, sizeof(int), 1, out_file);
    fwrite(&encoder_state.one_counter, sizeof(int), 1, out_file);
    fwrite(&encoder_state.current_index, sizeof(int), 1, out_file);
    fwrite(&encoder_state.out_index, sizeof(int), 1, out_file);
    fwrite(&encoder_state.last_symbol, sizeof(int), 1, out_file);
    fwrite(&encoder_state.current_symbol, sizeof(unsigned char), 1, out_file);
    fwrite(&encoder_state.base, sizeof(int), 1, out_file);
    fwrite(&encoder_state.length, sizeof(int), 1, out_file);



    fwrite(output, sizeof(unsigned char), output_size, out_file);
    printf("output cotent: %s\n", output);

    printf("actual_output_size: %zu\n", actual_output_size);

    //print all encoder_state
    printf("ac_state_t:\n");
    printf("  frac_size: %d\n", encoder_state.frac_size);
    printf("  one_counter: %d\n", encoder_state.one_counter);
    printf("  current_index: %d\n", encoder_state.current_index);
    printf("  out_index: %d\n", encoder_state.out_index);
    printf("  last_symbol: %d\n", encoder_state.last_symbol);
    printf("  current_symbol: %c\n", encoder_state.current_symbol);
    printf("  base: %d\n", encoder_state.base);
    printf("  length: %d\n", encoder_state.length);


    printf("Decoding...\n");
    unsigned char* decomp = calloc(sizeof(unsigned char) * input_size,1); // 解碼後的大小應該與輸入大小相同
    decode_value(decomp, output, &encoder_state, input_size);

    // 輸出結果
    printf("Decoded string: %s\n", decomp);


    fclose(out_file);
    free(decomp);
    free(output);



    //arithmetic_decompress(output_file);
}



void arithmetic_decompress(const char *input_file) {
    // 從 .arc 檔案中讀取資料
    //char input_arc_file[256];
    // 初始化編碼狀態
    ac_state_t encoder_state;

    init_state(&encoder_state, 16);

    printf("Decompressing %s using Arithmetic Coding...\n", input_file);

    FILE *in_file = fopen(input_file, "rb");
    if (in_file == NULL) {
        perror("Error opening input file");
        return;
    }

    size_t input_size;


    fread(&input_size, sizeof(size_t), 1, in_file);

    // 分配記憶體給 prob_table 和 cumul_table
    encoder_state.prob_table = malloc(sizeof(int) * 256); // 假設 prob_table 大小為 256
    encoder_state.cumul_table = malloc(sizeof(int) * 257); // 假設 cumul_table 大小為 256

    // 單獨讀取 encoder_state 的每個成員
    fread(encoder_state.prob_table, sizeof(int), 256, in_file);
    fread(encoder_state.cumul_table, sizeof(int), 257, in_file);
    fread(&encoder_state.frac_size, sizeof(int), 1, in_file);
    fread(&encoder_state.one_counter, sizeof(int), 1, in_file);
    fread(&encoder_state.current_index, sizeof(int), 1, in_file);
    fread(&encoder_state.out_index, sizeof(int), 1, in_file);
    fread(&encoder_state.last_symbol, sizeof(int), 1, in_file);
    fread(&encoder_state.current_symbol, sizeof(unsigned char), 1, in_file);
    fread(&encoder_state.base, sizeof(int), 1, in_file);
    fread(&encoder_state.length, sizeof(int), 1, in_file);


    //fread(&encoder_state, sizeof(ac_state_t), 1, in_file);

    size_t output_size = input_size*2; // 計算實際輸出大小
    unsigned char* output = calloc(sizeof(unsigned char) * output_size,1);
    unsigned char* decomp = calloc(sizeof(unsigned char) * input_size,1); // 解碼後的大小應該與輸入大小相同

    fread(output, sizeof(unsigned char), output_size , in_file);

    fclose(in_file);

    printf("Input size: %zu\n", input_size);
    
        //print all encoder_state
    printf("ac_state_t2:\n");
    printf("  frac_size: %d\n", encoder_state.frac_size);
    printf("  one_counter: %d\n", encoder_state.one_counter);
    printf("  current_index: %d\n", encoder_state.current_index);
    printf("  out_index: %d\n", encoder_state.out_index);
    printf("  last_symbol: %d\n", encoder_state.last_symbol);
    printf("  current_symbol: %c\n", encoder_state.current_symbol);
    printf("  base: %d\n", encoder_state.base);
    printf("  length: %d\n", encoder_state.length);
    printf("cumul %d",encoder_state.cumul_table[256]);


    printf("output cotent: %s\n", output);
    // 解碼
    printf("Decoding...\n");

    decode_value(decomp, output, &encoder_state, input_size);

    // 輸出結果
    printf("Decoded string: %s\n", decomp);

    // 釋放記憶體
    free(output);
    free(decomp);
}




// Function to display input prompt
void show_prompt()
{
    printf("Please enter the compression algorithm and input file:\n");
    printf("Usage: compress [algorithm] input_file\n");
}


// Main function
int main(int argc, char *argv[]) 
{
    char com_or_decom[100];
    char algorithm[100];
    char input_file[100];

    // Display input prompt
    show_prompt();

    // Read user input
    if (scanf("%99s %99s %99s",com_or_decom, algorithm, input_file) != 3) {
        return 1;
    }

    printf("Compress or Decompress: %s\n", com_or_decom);
    printf("Algorithm: %s\n", algorithm);
    printf("Input file: %s\n", input_file);

    arithmetic_decompress(input_file);

   /*
    // Open the input file for reading
    FILE *file = fopen(input_file, "rb");
    if (file == NULL) {
        perror("Error opening input file");
        return 1;
    }

    // Determine the file size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    // Allocate memory to hold the file content
    char *file_content = (char *)calloc(file_size, 1);

    if (file_content == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return 1;
    }

    // Read the file content
    fread(file_content, 1, file_size, file);
    printf("File content: %s\n", file_content);
    fclose(file);


    arithmetic_compress(file_content, file_size, input_file);

    
*/



    
/*
    // Select the appropriate compression algorithm
    if (strcmp(algorithm, "huffman") == 0) 
    {
        huffman_compress(file_content, file_size, input_file);
    } 
    else if (strcmp(algorithm, "arithmetic") == 0) 
    {
        arithmetic_compress(file_content, file_size, input_file);
    } 
    else 
    {
        printf("Error: Unknown algorithm '%s'.\n", algorithm);
        free(file_content);
        return 1;
    }
    */
    
    

    

    // Free allocated memory
    //free(file_content);
    
    


    return 0;
}


