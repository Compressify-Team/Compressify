#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
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

void arithmetic_compress(const char *file_content, size_t file_size, const char *input_file) 
{
    char output_file[256];
    snprintf(output_file, sizeof(output_file), "%s.arc", input_file);
    printf("Compressing %s to %s using Arithmetic Coding...\n", input_file, output_file);

    FILE *out_file = fopen(output_file, "wb");
    if (out_file == NULL) {
        perror("Error opening output file");
        return;
    }


    ac_state_t state;
    init_state(&state, 16); // 假設使用 16-bit fraction 範圍是 [0, 2^16)

    // 2. 建立機率表
    build_probability_table(&state, (unsigned char*)file_content, (int)file_size);

    // 3. 分配緩衝區
    //   預估壓縮後大小 (此處簡化直接分配 file_size bytes 空間)
    unsigned char *compressed_data = (unsigned char*)calloc(file_size, 1);

    if (!compressed_data) {
        perror("Memory allocation failed");
        fclose(out_file);
        return;
    }

    // 4. 執行編碼
    encode_value(compressed_data, (unsigned char*)file_content, file_size, &state);

    // 5. 計算實際使用的位元 (state.out_index)，轉為位元組並寫入檔案
    int total_bits = state.out_index;
    int total_bytes = (total_bits + 7) / 8; // 向上取整
    fwrite(compressed_data, 1, total_bytes, out_file);

    // 解壓縮並檢查結果
    arithmetic_decompress_memory(compressed_data, total_bytes, file_size);


    // 清理
    free(compressed_data);
    free(state.prob_table);
    free(state.cumul_table);
    fclose(out_file);

    // 解壓縮並檢查結果
    /*
    char decompressed_output_file[256];
    snprintf(decompressed_output_file, sizeof(decompressed_output_file), "%s.txt", input_file);
    arithmetic_decompress(output_file, decompressed_output_file);
    */
    
}

void arithmetic_decompress_memory(const unsigned char *compressed_data, size_t compressed_size, size_t original_size) {
    printf("Decompressing from memory using Arithmetic Coding...\n");

    ac_state_t state;
    init_state(&state, 16); // 假設使用 16-bit fraction

    unsigned char *decompressed_data = (unsigned char*)malloc(original_size);
    if (!decompressed_data) {
        perror("Memory allocation failed");
        return;
    }

    decode_value(decompressed_data, compressed_data, &state, original_size);

    // 顯示解壓縮後的結果
    printf("Decompressed data: %s\n", decompressed_data);

    free(decompressed_data);
}

/*
void arithmetic_decompress(const char *input_file, const char *output_file) {
    printf("Decompressing %s to %s using Arithmetic Coding...\n", input_file, output_file);

    FILE *in_file = fopen(input_file, "rb");
    if (in_file == NULL) {
        perror("Error opening input file");
        return;
    }



    fseek(in_file, 0, SEEK_END);
    size_t file_size = ftell(in_file);
    fseek(in_file, 0, SEEK_SET);



    unsigned char *compressed_data = (unsigned char*)calloc(file_size, 1);
    if (!compressed_data) {
        perror("Memory allocation failed");
        fclose(in_file);
        return;
    }

    size_t read_size = fread(compressed_data, 1, file_size, in_file);
    if (read_size != file_size) {
        perror("Error reading input file");
        free(compressed_data);
        fclose(in_file);
        return;
    }

    fclose(in_file);

    ac_state_t state;
    init_state(&state, 16); // 假設使用 16-bit fraction

    unsigned char *decompressed_data = (unsigned char*)malloc(file_size * 2); // 假設解壓後大小不超過原大小的兩倍
    if (!decompressed_data) {
        perror("Memory allocation failed");
        free(compressed_data);
        return;
    }

    decode_value(decompressed_data, compressed_data, &state, file_size * 2);

    FILE *out_file = fopen(output_file, "wb");
    if (out_file == NULL) {
        perror("Error opening output file");
        free(compressed_data);
        free(decompressed_data);
        return;
    }

    fwrite(decompressed_data, 1, state.out_index, out_file);

    free(compressed_data);
    free(decompressed_data);
    fclose(out_file);
}

*/

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

    char output_file[100]="output.txt";

    //arithmetic_decompress(input_file, output_file);


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

    

    // Free allocated memory
    free(file_content);


    return 0;
}


