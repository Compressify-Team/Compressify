#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "arith_cod.h"
#include <sndfile.h>
#include <fftw3.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/time.h>
#define MAX_CHAR 256

typedef struct Node {
    char val;
    int freq;
    struct Node* lchild;
    struct Node* rchild;
} Node;

// Frequency table and forest array
int freq[MAX_CHAR] = {0};
Node* forest[MAX_CHAR];
int forest_size = 0;

// New function to flush remaining bits
void flushBitBuffer(FILE* out_file, unsigned char* buffer, int* buffer_size) {
    if (*buffer_size > 0) {
        // Left-shift the remaining bits
        *buffer = *buffer << (8 - *buffer_size);
        fputc(*buffer, out_file);
        *buffer_size = 0;
        *buffer = 0;
    }
}

// Modified writeBit function
void writeBit(FILE* out_file, unsigned char* buffer, int* buffer_size, int bit) {
    *buffer = (*buffer << 1) | (bit & 1);
    (*buffer_size)++;

    // If the buffer is full, write it to the file
    if (*buffer_size == 8) {
        fputc(*buffer, out_file);
        *buffer_size = 0;
        *buffer = 0;
    }
}

// Function to serialize the Huffman Tree using pre-order traversal and bit-level storage
void serializeTree(Node* root, FILE* out_file, unsigned char* buffer, int* buffer_size) {
    if (!root) return;

    if (!root->lchild && !root->rchild) {
        //printf("Serializing leaf node: %c\n", root->val); // Debug: Output leaf node value
        // Write '1' to indicate a leaf node and write the character
        writeBit(out_file, buffer, buffer_size, 1);
         // Write the ASCII value of the leaf node bit by bit
        for (int i = 7; i >= 0; i--) { // Write 8 bits for the ASCII value
            int bit = (root->val >> i) & 1;
            writeBit(out_file, buffer, buffer_size, bit);
        }
        } else {
        // Write '0' to indicate an internal node
        writeBit(out_file, buffer, buffer_size, 0);
    }

    serializeTree(root->lchild, out_file, buffer, buffer_size);
    serializeTree(root->rchild, out_file, buffer, buffer_size);
}

// Function to create a new node
Node* createNode(char val, int freq, Node* lchild, Node* rchild) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->val = val;
    node->freq = freq;
    node->lchild = lchild;
    node->rchild = rchild;
    return node;
}

// Comparator function for sorting
int compare(const void* a, const void* b) {
    Node* nodeA = *(Node**)a;
    Node* nodeB = *(Node**)b;
    return nodeA->freq - nodeB->freq;
}

// Function to find the Huffman code for a specific character
void findCode(Node* node, char* code, int depth, char target, char* result) {
    if (!node->lchild && !node->rchild) {
        if (node->val == target) {
            //printf("Character '%c' found in tree.\n", target); // Debug: Output character found in tree
            if (depth == 0) {
                // Special case: if the tree has only one character, its code should be '0'
                code[depth] = '0';
                code[depth + 1] = '\0';
            }
            else {
                code[depth] = '\0';
            }
            //printf("Code for character '%c': %s\n", target, code); // Debug: Output code for the character
            strcpy(result, code);
        }
        return;
    }
    if (node->lchild) {
        code[depth] = '0';
        findCode(node->lchild, code, depth + 1, target, result);
    }
    if (node->rchild) {
        code[depth] = '1';
        findCode(node->rchild, code, depth + 1, target, result);
    }
}

// Function to encode the input string using the Huffman tree
unsigned char* encode(Node* root, const char* input, size_t* encoded_len) {
    //printf("Input: %s\n", input);
    char code[256] = {0};
    char result[256] = {0};
    size_t len = strlen(input);
    size_t max_bits = len * 8; // Maximum possible number of bits
    unsigned char* encoded = (unsigned char*)malloc(max_bits / 8 + 1); // Allocate space for bits
    if (encoded == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }

    size_t bit_pos = 0;
    for (size_t i = 0; input[i] != '\0'; i++) {
        findCode(root, code, 0, input[i], result);
        //printf("Character '%c' encoded as %s\n", input[i], result); // Debug: Output encoded character and its code
        // Add the encoded bits to the buffer
        for (size_t j = 0; result[j] != '\0'; j++) {
            if (bit_pos % 8 == 0) {
                // Move to the next byte if necessary
                encoded[bit_pos / 8] = 0;
            }
            encoded[bit_pos / 8] |= (result[j] - '0') << (7 - (bit_pos % 8));
            bit_pos++;
        }
    }
    *encoded_len = (bit_pos + 7) / 8;  // Round up to the nearest byte
    return encoded;
}

// Modified deserializeTree function to handle partial bytes
Node* deserializeTree(FILE* in_file) {
    static unsigned char buffer = 0;
    static int bit_pos = 0;

    // Read the next bit from the input file
    unsigned char readBit() {
        if (bit_pos == 0) {
            buffer = fgetc(in_file);
            if (feof(in_file)) {
                fprintf(stderr, "Error: Unexpected end of file while reading bits.\n");
                exit(1);
            }
            bit_pos = 8;
        }
        bit_pos--;
        return (buffer >> bit_pos) & 1;
    }

    int bit = readBit();
    if (bit == 1) {
        // Leaf node: read the character bit by bit
        char val = 0;
        for (int i = 0; i < 8; i++) {
            val = (val << 1) | readBit();
        }
        //printf("Deserialized leaf node: %c\n", val); // Debug: Output leaf node value
        return createNode(val, 0, NULL, NULL);
    } 
    else {
        // Internal node
        Node* left = deserializeTree(in_file);
        Node* right = deserializeTree(in_file);
        return createNode(-1, 0, left, right);
    }
}

void decompress_huffman(const char *input_file) {
    // Open the compressed file for reading
    FILE *in_file = fopen(input_file, "rb");
    if (in_file == NULL) {
        perror("Error opening input file");
        return;
    }

    // Deserialize the Huffman tree from the compressed file
    printf("Deserializing Huffman tree...\n");
    Node* root = deserializeTree(in_file);
    printf("Huffman tree deserialization complete.\n");

    // Open output file for writing
    FILE *out_file = fopen("output_decoded.txt", "wb");
    if (out_file == NULL) {
        perror("Error opening output file");
        fclose(in_file);
        return;
    }

    unsigned char buffer = 0;
    int bit_pos = 0;
    Node* current = root;

    // Decode the content bit by bit
    printf("Starting decompression...\n");
    while (1) {
        if (bit_pos == 0) {
            buffer = fgetc(in_file);
            if (feof(in_file)) break; // End of file
            bit_pos = 8;
        }

        int bit = (buffer >> (bit_pos - 1)) & 1;
        bit_pos--;
        //printf("Read bit: %d\n", bit); // Debug: Output bit value
        // Traverse the tree based on the bit
        if (bit == 0) {
            current = current->lchild;
        } 
        else {
            current = current->rchild;
        }

        // If a leaf node is reached, write the character to the output file
        if (!current->lchild && !current->rchild) {
            //printf("Decoded character: %c\n", current->val); // Debug: Output decoded character
            fputc(current->val, out_file);
            current = root; // Reset to root for the next character
        }
    }

    // Free the Huffman tree (post-order traversal)
    Node* stack[MAX_CHAR];
    int stack_size = 0;
    Node* last_visited = NULL;
    Node* root_node = root;

    while (stack_size > 0 || root_node) {
        if (root_node) {
            stack[stack_size++] = root_node;
            root_node = root_node->lchild;
        } 
        else {
            Node* peek_node = stack[stack_size - 1];
            if (peek_node->rchild && last_visited != peek_node->rchild) {
                root_node = peek_node->rchild;
            } else {
                //printf("Freeing node: %c\n", peek_node->val); // Debug: Output node being freed
                //free(peek_node);
                last_visited = peek_node;
                stack_size--;
            }
        }
    }

    // Close files
    fclose(in_file);
    fclose(out_file);
    printf("Decompression complete. Output written to 'output_decoded.txt'.\n");
}


// Function to perform Huffman compression
int huffman_compress(const char *file_content, const char *input_file) {
    size_t len = strlen(file_content); 
    char output_file[256];
    snprintf(output_file, sizeof(output_file), "%s.huf", input_file);
    printf("Compressing %s to %s using Huffman coding...\n", input_file, output_file);

    // Calculate frequency of each character
    memset(freq, 0, sizeof(freq));  // Reset frequency array
    for (size_t i = 0; i < len; i++) {
        freq[(unsigned char)file_content[i]]++;
    }

    // Create nodes for characters with non-zero frequencies
    for (int i = 0; i < MAX_CHAR; i++) {
        if (freq[i] > 0) {
            forest[forest_size++] = createNode(i, freq[i], NULL, NULL);
        }
    }

    // Build the Huffman tree
    while (forest_size > 1) {
        qsort(forest, forest_size, sizeof(Node*), compare);
        Node* left = forest[0];
        Node* right = forest[1];
        Node* parent = createNode(-1, left->freq + right->freq, left, right);
        forest[0] = parent;
        for (int i = 1; i < forest_size - 1; i++) {
            forest[i] = forest[i + 1];
        }
        forest_size--;
    }

    // Open output file for writing
    FILE *out_file = fopen(output_file, "wb");
    if (out_file == NULL) {
        perror("Error opening output file");
        return 0;
    }

    unsigned char buffer = 0;
    int buffer_size = 0;

    // Serialize the Huffman tree with bit-level storage
    serializeTree(forest[0], out_file, &buffer, &buffer_size);
    
    // Flush any remaining bits from tree serialization
    flushBitBuffer(out_file, &buffer, &buffer_size);

    // Reset buffer for encoding content
    buffer = 0;
    buffer_size = 0;

    //Encode the file content and write to the output file
    size_t encoded_len = 0;
    unsigned char* encoded_content = encode(forest[0], file_content, &encoded_len);
    if (!encoded_content) {
        perror("Encoding failed");
        fclose(out_file);
        return 0;
    }
    // Print encoded content in binary form
    // printf("Encoded content in binary form:\n");
    // for (size_t i = 0; i < encoded_len; i++) {
    //     for (int j = 7; j >= 0; j--) {
    //         printf("%d", (encoded_content[i] >> j) & 1);
    //     }
    //     printf(" ");
    // }
    // printf("\n");
    // // print encoded_len 
    // printf("Encoded length: %zu\n", encoded_len);
    // // Write the encoded content to the file
    fwrite(encoded_content, 1, encoded_len, out_file);

    // Free the allocated memory (post-order traversal to free nodes)
    Node* stack[MAX_CHAR];
    int stack_size = 0;
    Node* last_visited = NULL;
    Node* root = forest[0];

    while (stack_size > 0 || root) {
        if (root) {
            stack[stack_size++] = root;
            root = root->lchild;
        } 
        else {
            Node* peek_node = stack[stack_size - 1];
            if (peek_node->rchild && last_visited != peek_node->rchild) {
                root = peek_node->rchild;
            } 
            else {
                //free(peek_node);
                last_visited = peek_node;
                stack_size--;
            }
        }
    }
    //calculate the size of the compressed file
    fseek(out_file, 0, SEEK_END);
    size_t compressed_size = ftell(out_file);
    free(encoded_content);
    fclose(out_file);
    return compressed_size;
}














//-------------------------------------------arithmetic coding-------------------------------------------
size_t calculate_output_size(const unsigned char *output, size_t max_size) {
    size_t size = 0;
    while (size < max_size && output[size] != '\0') {
        size++;
    }
    return size;
}

int arithmetic_compress(const char *file_content, const char *input_file) {
    size_t input_size = strlen(file_content);

    size_t output_size = input_size * 2; // 預估大小

    unsigned char* input = (unsigned char*)file_content;
    unsigned char* output = calloc(sizeof(unsigned char) * output_size, 1);

    // initialize the encoder state
    ac_state_t encoder_state;
    init_state(&encoder_state, 16);

    // build the probability table
    build_probability_table(&encoder_state, input, input_size);

    printf("Encoding...\n");

    encode_value(output, input, input_size, &encoder_state);


    char output_file[256];

    snprintf(output_file, sizeof(output_file), "%s.arc", input_file);
    FILE *out_file = fopen(output_file, "wb");
    if (out_file == NULL) {
        perror("Error opening output file");
        free(output);
        return 0;
    }
    

    size_t actual_output_size = calculate_output_size(output, output_size);

    fwrite(&input_size, sizeof(size_t), 1, out_file);
    printf("input_size: %zu\n", input_size);

    //fwrite(encoder_state.prob_table, sizeof(int), 256, out_file); 
    fwrite(encoder_state.cumul_table, sizeof(int), 128, out_file); 
    //fwrite(&encoder_state.frac_size, sizeof(int), 1, out_file);
    //fwrite(&encoder_state.one_counter, sizeof(int), 1, out_file);
    //fwrite(&encoder_state.current_index, sizeof(int), 1, out_file);
    //fwrite(&encoder_state.out_index, sizeof(int), 1, out_file);
    //fwrite(&encoder_state.last_symbol, sizeof(int), 1, out_file);
    //fwrite(&encoder_state.current_symbol, sizeof(unsigned char), 1, out_file);
    fwrite(&encoder_state.base, sizeof(int), 1, out_file);
    //fwrite(&encoder_state.length, sizeof(int), 1, out_file);

    fwrite(output, sizeof(unsigned char), actual_output_size, out_file);

    //printf("output cotent: %s\n", output);
    //printf("output_size: %zu\n", actual_output_size);
    /*
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
*/

    //calculate the size of the compressed file
    fseek(out_file, 0, SEEK_END);
    size_t compressed_size = ftell(out_file);
    fclose(out_file);
    free(output);
    return compressed_size;


}



void arithmetic_decompress(const char *input_file) {

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


    encoder_state.prob_table = malloc(sizeof(int) * 128); 
    encoder_state.cumul_table = malloc(sizeof(int) * 128); 


    //fread(encoder_state.prob_table, sizeof(int), 256, in_file);
    fread(encoder_state.cumul_table, sizeof(int),128, in_file);
    //fread(&encoder_state.frac_size, sizeof(int), 1, in_file);
    //fread(&encoder_state.one_counter, sizeof(int), 1, in_file);
    //fread(&encoder_state.current_index, sizeof(int), 1, in_file);
    //fread(&encoder_state.out_index, sizeof(int), 1, in_file);
    //fread(&encoder_state.last_symbol, sizeof(int), 1, in_file);
    //fread(&encoder_state.current_symbol, sizeof(unsigned char), 1, in_file);
    fread(&encoder_state.base, sizeof(int), 1, in_file);
    //fread(&encoder_state.length, sizeof(int), 1, in_file);

    size_t output_size = input_size*2; 
    unsigned char* output = calloc(sizeof(unsigned char) * output_size,1);
    unsigned char* decomp = calloc(sizeof(unsigned char) * input_size,1); 

    fread(output, sizeof(unsigned char), output_size , in_file);

    fclose(in_file);
    /*
    printf("Input size: %zu\n", input_size);
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
    */
    
    printf("Decoding...\n");

    decode_value(decomp, output, &encoder_state, input_size);
    printf("Decoded string: %s\n", decomp);

    char output_file[256];
    strncpy(output_file, input_file, sizeof(output_file) - 1);
    output_file[sizeof(output_file) - 1] = '\0';

    // 去掉 .txt.huf 
    char *dot = strrchr(output_file, '.');
    if (dot && strcmp(dot, ".huf") == 0) {
        *dot = '\0';
        dot = strrchr(output_file, '.');
        if (dot && strcmp(dot, ".txt") == 0) {
            *dot = '\0';
        }
    }

    strncat(output_file, "_arithmetic.txt", sizeof(output_file) - strlen(output_file) - 1);

    // 寫檔
    FILE *out_file = fopen(output_file, "wb");
    if (out_file == NULL) {
        perror("Error opening output file");
        free(output);
        free(decomp);
        free(encoder_state.prob_table);
        free(encoder_state.cumul_table);
        return;
    }
    fwrite(decomp, 1, input_size, out_file);
    fclose(out_file);
    printf("Decoded content written to %s\n", output_file);

    free(output);
    free(decomp);
}

//-------------------------------------------arithmetic coding end-------------------------------------------

//-------------------------------------------Audio Compression-------------------------------------------


void print_progress(double percentage) {
    int barWidth = 70;
    printf("[");
    int pos = barWidth * percentage;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %d %%\r", (int)(percentage * 100.0));
    fflush(stdout);
}

void compress_audio(const char *input_file, const char *output_file) {
    // Open input file
    SF_INFO sfinfo;
    SNDFILE *infile = sf_open(input_file, SFM_READ, &sfinfo);
    if (!infile) {
        printf("Failed to open input file\n");
        return;
    }

    // Read audio data
    int num_samples = sfinfo.frames * sfinfo.channels;
    double *buffer = (double *)malloc(num_samples * sizeof(double));
    sf_read_double(infile, buffer, num_samples);

    // Perform FFT
    fftw_complex *out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * num_samples);
    fftw_plan p = fftw_plan_dft_r2c_1d(num_samples, buffer, out, FFTW_ESTIMATE);
    fftw_execute(p);

    // Write compressed data to output file
    FILE *outfile = fopen(output_file, "wb");
    fwrite(&sfinfo, sizeof(SF_INFO), 1, outfile); // Write SF_INFO to output file
    fwrite(out, sizeof(fftw_complex), num_samples, outfile);

    // Calculate compression ratio
    long original_size = num_samples * sizeof(double);
    long compressed_size = sizeof(SF_INFO) + num_samples * sizeof(fftw_complex);
    double compression_ratio = (double)compressed_size / original_size * 100.0;
    printf("\nOriginal size: %ld bytes\n", original_size);
    printf("Compressed size: %ld bytes\n", compressed_size);
    printf("Compression ratio: %.2f%%\n", compression_ratio);

    // Clean up
    fclose(outfile);
    fftw_destroy_plan(p);
    fftw_free(out);
    sf_close(infile);
    free(buffer);
}

void decompress_audio(const char *input_file, const char *output_file) {
    // Open input file
    FILE *infile = fopen(input_file, "rb");
    if (!infile) {
        printf("Failed to open input file\n");
        return;
    }

    // Read SF_INFO from input file
    SF_INFO sfinfo;
    fread(&sfinfo, sizeof(SF_INFO), 1, infile);

    // Read compressed data
    fseek(infile, 0, SEEK_END);
    long file_size = ftell(infile) - sizeof(SF_INFO);
    fseek(infile, sizeof(SF_INFO), SEEK_SET);
    fftw_complex *buffer = (fftw_complex *)fftw_malloc(file_size);
    fread(buffer, 1, file_size, infile);

    // Perform inverse FFT
    int num_samples = file_size / sizeof(fftw_complex);
    double *out = (double *)malloc(num_samples * sizeof(double));
    fftw_plan p = fftw_plan_dft_c2r_1d(num_samples, buffer, out, FFTW_ESTIMATE);
    fftw_execute(p);

    // Normalize the output from IFFT
    for (int i = 0; i < num_samples; i++) {
        out[i] /= num_samples;
    }

    // Write decompressed data to output file
    SNDFILE *outfile = sf_open(output_file, SFM_WRITE, &sfinfo);
    sf_write_double(outfile, out, num_samples);

    // Calculate decompressed size
    long decompressed_size = num_samples * sizeof(double);
    double decompression_ratio = (double)decompressed_size / file_size * 100.0;
    printf("\nDecompressed size: %ld bytes\n", decompressed_size);
    printf("Decompression ratio: %.2f%%\n", decompression_ratio);

    // Clean up
    sf_close(outfile);
    fftw_destroy_plan(p);
    fftw_free(buffer);
    fclose(infile);
    free(out);
}

// void clear_input_buffer() {
//     int c;
//     while ((c = getchar()) != '\n' && c != EOF);
// }


//-------------------------------------------Audio Compression end-------------------------------------------
// Function to display input prompt
void show_main_menu() {
    printf("Please select an option:\n");
    printf("1. Compress\n");
    printf("2. Decompress\n");
    printf("3. Exit\n");
}

void show_compress_menu() {
    printf("Please select a compression algorithm:\n");
    printf("1. Huffman\n");
    printf("2. Arithmetic\n");
    printf("3. Audio\n");
    printf("4. Auto\n");
    printf("5. Back to main menu\n");
}

void show_decompress_menu() {
    printf("Please select a decompression algorithm:\n");
    printf("1. Huffman\n");
    printf("2. Arithmetic\n");
    printf("3. Audio\n");
    printf("4. Auto\n");
    printf("5. Back to main menu\n");
}
// void show_prompt()
// {
//     printf("Please enter the decompression/compression algorithm and input file or output file if :\n");
//     printf("Algorithms can be used: huffman, arithmetic or audio \n");
//     printf("Usage: -c/-d [algorithm] \n");
// }
char com_or_decom[100];
char algorithm[100];
typedef struct {
    char *file_content;
    size_t file_size;
} FileData;
FileData fileOpen(const char *input_file) {
    FileData file_data = {NULL, 0};
    printf("Input file: %s\n", input_file);
    FILE *file = fopen(input_file, "rb");
    if (file == NULL) {
        perror("Error opening input file");
        return file_data;
    }
    fseek(file, 0, SEEK_END);
    file_data.file_size = ftell(file);
    rewind(file);
    file_data.file_content = (char *)calloc(file_data.file_size, 1);
    if (file_data.file_content == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return file_data;
    }
    fread(file_data.file_content, 1, file_data.file_size, file);
    fclose(file);
    return file_data;
}
// Main function
int main() {
    int main_choice, sub_choice;
    while (1) {
        // Display main menu
        show_main_menu();
        printf("Enter your choice: ");
        scanf("%d", &main_choice);

        if (main_choice == 3) {
            printf("Exiting the program.\n");
            break;
        }

        if (main_choice == 1) {
            // Compression menu
            while (1) {
                show_compress_menu();
                printf("Enter your choice: ");
                scanf("%d", &sub_choice);

                if (sub_choice == 5) {
                    break;
                }

                char input_file[256], output_file[256];
                printf("請輸入輸入檔案名稱: ");
                scanf("%s", input_file);

                if (sub_choice == 1) {
                    FileData file_data = fileOpen(input_file);
                    if (file_data.file_content == NULL) {
                        return 1;
                    }
                    int compressed_size = huffman_compress(file_data.file_content, input_file);
                    if (compressed_size != 0) {
                        printf("Compressed file size: %d bytes\n", compressed_size);
                        double compression_ratio = (double)compressed_size / (double)file_data.file_size * 100.0;
                        printf("Compression ratio: %.2f%%\n", compression_ratio);
                    }
                    free(file_data.file_content);
                } else if (sub_choice == 2) {
                    FileData file_data = fileOpen(input_file);
                    if (file_data.file_content == NULL) {
                        return 1;
                    }
                    int compressed_size = arithmetic_compress(file_data.file_content, input_file);
                    if (compressed_size != 0) {
                        printf("Compressed file size: %d bytes\n", compressed_size);
                        double compression_ratio = (double)compressed_size / (double)file_data.file_size * 100.0;
                        printf("Compression ratio: %.2f%%\n", compression_ratio);
                    }
                    free(file_data.file_content);
                } else if (sub_choice == 3) {
                    printf("請輸入輸出檔案名稱: ");
                    scanf("%s", output_file);
                    compress_audio(input_file, output_file);
                } else if (sub_choice == 4) {

                    // Check if the filename ends with .txt or .wav
                    if (strstr(input_file, ".txt") != NULL) {
                        printf("The file is a .txt file.\n");
                        FileData file_data = fileOpen(input_file);
                        if (file_data.file_content == NULL) {
                            return 1;
                        }

                        int compressed_size_huf = huffman_compress(file_data.file_content, input_file);
                        double compression_ratio_huf = 0;
                        if (compressed_size_huf != 0) {
                            printf("Compressed file size: %d bytes\n", compressed_size_huf);
                            compression_ratio_huf = (double)compressed_size_huf / (double)file_data.file_size * 100.0;
                            printf("Compression ratio: %.2f%%\n", compression_ratio_huf);
                        }

                        int compressed_size_arc = arithmetic_compress(file_data.file_content, input_file);
                        double compression_ratio_arc = 0;
                        if (compressed_size_arc != 0) {
                        printf("Compressed file size: %d bytes\n", compressed_size_arc);
                        compression_ratio_arc = (double)compressed_size_arc / (double)file_data.file_size * 100.0;
                        printf("Compression ratio: %.2f%%\n", compression_ratio_arc);
                        }

                        printf("--------------------------------------------------------------\n\n\n");
                        printf("huffman compressed ratio: %.2f%%\n", compression_ratio_huf);
                        printf("arithmetic compressed ratio: %.2f%%\n", compression_ratio_arc);
                        printf("--------------------------------------------------------------\n");
                        if (compression_ratio_huf > compression_ratio_arc) {
                            printf("arithmetic compressed ratio is better\n");
                        } else{
                            printf("huffman compressed ratio is better\n");
                        }
                        printf("--------------------------------------------------------------\n");                        
                        printf("Which would you prefer\n");
                        printf("1. Huffman\n");
                        printf("2. Arithmetic\n");
                        printf("3. Both\n");
                        printf("4. None\n");
                        printf("Enter your choice: ");
                        int choice;
                        scanf("%d", &choice);
                        if (choice == 1) {
                            printf("Compressed file size: %d bytes\n", compressed_size_huf);
                            printf("Compression ratio: %.2f%%\n", compression_ratio_huf);
                            // Delete the file with .arc extension
                            char arc_file[256];
                            snprintf(arc_file, sizeof(arc_file), "%s.arc", input_file);
                            if (remove(arc_file) != 0) {
                                printf("Error deleting the file %s.\n", arc_file);
                            }
                        } else if (choice == 2) {
                            printf("Compressed file size: %d bytes\n", compressed_size_arc);
                            printf("Compression ratio: %.2f%%\n", compression_ratio_arc);

                            // Delete the file with .huf extension
                            char huf_file[256];
                            snprintf(huf_file, sizeof(huf_file), "%s.huf", input_file);
                            if (remove(huf_file) != 0) {
                                printf("Error deleting the file %s.\n", huf_file);
                            }
                        } else if (choice == 3) {
                            printf("Compressed file size (Huffman): %d bytes\n", compressed_size_huf);
                            printf("Compression ratio (Huffman): %.2f%%\n", compression_ratio_huf);
                            printf("Compressed file size (Arithmetic): %d bytes\n", compressed_size_arc);
                            printf("Compression ratio (Arithmetic): %.2f%%\n", compression_ratio_arc);
                        } else if (choice == 4) {
                            printf("No compression chosen.\n");
                        } else {
                            printf("Invalid choice. Please try again.\n");
                        }
                    
                        free(file_data.file_content);
                    
                    } else if (strstr(input_file, ".wav") != NULL) {
                        printf("請輸入輸出檔案名稱: ");
                        scanf("%s", output_file);
                        compress_audio(input_file, output_file);
                    } else {
                        printf("The file is neither .txt nor .wav.\n");
                    }
                } else {
                    printf("Invalid choice. Please try again.\n");
                }
            }


        } else if (main_choice == 2) {
            // Decompression menu
            while (1) {
                show_decompress_menu();
                printf("Enter your choice: ");
                scanf("%d", &sub_choice);

                if (sub_choice == 5) {
                    break;
                }

                char input_file[512], output_file[512];
                printf("請輸入輸入檔案名稱: ");
                scanf("%s", input_file);

                if (sub_choice == 1) {
                    printf("Decompressing %s using Huffman Coding...\n", input_file);
                    decompress_huffman(input_file);
                } else if (sub_choice == 2) {
                    arithmetic_decompress(input_file);
                } else if (sub_choice == 3) {
                    printf("請輸入輸出檔案名稱: ");
                    scanf("%s", output_file);
                    decompress_audio(input_file, output_file);
                } else if (sub_choice == 4)
                {
                    // Check if the filename ends with .txt or .bin
                    if (strstr(input_file, ".arc") != NULL) {
                        printf("The file is a .arc file.\n");
                        arithmetic_decompress(input_file);

                    } else if (strstr(input_file, ".huf") != NULL) {
                        printf("The file is a .huf file.\n");
                        decompress_huffman(input_file);
                    } 
                    else if (strstr(input_file, ".bin") != NULL) {
                        printf("The file is a .bin file.\n");
                        printf("請輸入輸出檔案名稱: ");
                        scanf("%s", output_file);
                        decompress_audio(input_file, output_file);
                    } else {
                        printf("The file is neither arc,huf nor bin.\n");
                    }
                }
                else {
                    printf("Invalid choice. Please try again.\n");
                }
            }
        } else {
            printf("Invalid choice. Please try again.\n");
        }
    }
    return 0;
}
// Main function
// int main() {
//     while (1) {
//         // Display input prompt
//         show_prompt();
//         struct rusage usage;
//         struct timeval start, end;
//         gettimeofday(&start, NULL);

//         // Read user input
//         if (scanf("%99s %99s", com_or_decom, algorithm) != 2) {
//             return 1;
//         }

//         if (strcmp(com_or_decom, "-c") == 0) {
//             printf("Compress or Decompress: compress\n");
//         } else if (strcmp(com_or_decom, "-d") == 0) {
//             printf("Compress or Decompress: decompress\n");
//         } else {
//             printf("Error: Unknown operation '%s'. Use -c for compression or -d for decompression.\n", com_or_decom);
//             return 1;
//         }
//         printf("Algorithm: %s\n", algorithm);

//         // Select the appropriate action based on user input
//         if (strcmp(com_or_decom, "-c") == 0) {
//             // Compression
//             if (strcmp(algorithm, "huffman") == 0) {
//                 char input_file[100];
//                 printf("請輸入輸入檔案名稱: ");
//                 scanf("%s", input_file);
//                 FileData file_data = fileOpen(input_file);
//                 if (file_data.file_content == NULL) {
//                     return 1;
//                 }
//                 int compressed_size = huffman_compress(file_data.file_content, input_file);
//                 if (compressed_size != 0) {
//                     printf("Compressed file size: %d bytes\n", compressed_size);
//                     double compression_ratio = (double)compressed_size / (double)file_data.file_size;
//                     printf("Compression ratio: %.2f%%\n", compression_ratio);
//                 }
//                 free(file_data.file_content);
//             } else if (strcmp(algorithm, "arithmetic") == 0) {
//                 char input_file[100];
//                 printf("請輸入輸入檔案名稱: ");
//                 scanf("%s", input_file);
//                 FileData file_data = fileOpen(input_file);
//                 if (file_data.file_content == NULL) {
//                     return 1;
//                 }
//                 arithmetic_compress(file_data.file_content, input_file);
//                 free(file_data.file_content);
//             } else if (strcmp(algorithm, "audio") == 0) {
//                 char input_file[256], output_file[256];
//                 printf("請輸入輸入檔案名稱: ");
//                 scanf("%s", input_file);
//                 printf("請輸入輸出檔案名稱: ");
//                 scanf("%s", output_file);
//                 compress_audio(input_file, output_file);
//                 gettimeofday(&end, NULL);
//                 getrusage(RUSAGE_SELF, &usage);
//                 double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
//                 printf("Elapsed time: %.2f%% seconds\n", elapsed_time);
//                 printf("Memory usage: %ld kilobytes\n", usage.ru_maxrss);
//             } else {
//                 printf("Error: Unknown algorithm '%s'.\n", algorithm);
//                 return 1;
//             }
//         } else if (strcmp(com_or_decom, "-d") == 0) {
//             // Decompression
//             if (strcmp(algorithm, "huffman") == 0) {
//                 char input_file[100];
//                 printf("請輸入輸入檔案名稱: ");
//                 scanf("%s", input_file);
//                 printf("Decompressing %s using Huffman Coding...\n", input_file);
//                 decompress_huffman(input_file);
//             } else if (strcmp(algorithm, "arithmetic") == 0) {
//                 char input_file[100];
//                 printf("請輸入輸入檔案名稱: ");
//                 scanf("%s", input_file);
//                 arithmetic_decompress(input_file);
//             } else if (strcmp(algorithm, "audio") == 0) {
//                 char input_file[256], output_file[256];
//                 printf("請輸入輸入檔案名稱: ");
//                 scanf("%s", input_file);
//                 printf("請輸入輸出檔案名稱: ");
//                 scanf("%s", output_file);
//                 decompress_audio(input_file, output_file);
//                 gettimeofday(&end, NULL);
//                 getrusage(RUSAGE_SELF, &usage);
//                 double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
//                 printf("Elapsed time: %.2f%% seconds\n", elapsed_time);
//                 printf("Memory usage: %ld kilobytes\n", usage.ru_maxrss);
//             } else {
//                 printf("Error: Unknown algorithm '%s'.\n", algorithm);
//                 return 1;
//             }
//         } else {
//             printf("Error: Unknown operation '%s'. Use -c for compression or -d for decompression.\n", com_or_decom);
//             return 1;
//         }

//         printf("\n\n\n");
//     }
//     return 0;
// }
//---------------------------------------------------------------------------------
// Main function

// int main() 
// {
//     while (1)
//     {
//         // Display input prompt
//         show_prompt();
//         struct rusage usage;
//         struct timeval start, end;
//         gettimeofday(&start, NULL);

//         // Read user input
//         if (scanf("%99s %99s ", com_or_decom, algorithm) != 2) {
//             return 1;
//         }
//         if (strcmp(com_or_decom,"-c"))
//         {
//             printf("Compress or Decompress: compress\n");
//         }
//         else if (strcmp(com_or_decom,"-d"))
//         {
//             printf("Compress or Decompress: decompress\n");
//         }
//         else
//         {
//             printf("Error: Unknown operation '%s'. Use -c for compression or -d for decompression.\n", com_or_decom);
//             return 1;
//         }
//         printf("Algorithm: %s\n", algorithm);

        

//         // Select the appropriate action based on user input
//         if (strcmp(com_or_decom, "-c") == 0) {
//             // Compression
//             if (strcmp(algorithm, "huffman") == 0) {
//                 fileOpen();
//                 int compressed_size = huffman_compress(file_content, input_file);
//                 if(compressed_size != 0){
//                     printf("Compressed file size: %d bytes\n", compressed_size);
//                     double compression_ratio = (double)compressed_size / (double)file_size;
//                     printf("Compression ratio: %.2f%%\n", compression_ratio);
//                 }
//             } else if (strcmp(algorithm, "arithmetic") == 0) {
//                 fileOpen();
//                 arithmetic_compress(file_content, input_file);
//             } else if (strcmp(algorithm, "audio") == 0) {
//                 char input_file[256], output_file[256];
//                 printf("請輸入輸入檔案名稱: ");
//                 scanf("%s", input_file);
//                 printf("請輸入輸出檔案名稱: ");
//                 scanf("%s", output_file);
//                 compress_audio(input_file, output_file);
//                 gettimeofday(&end, NULL);
//                 getrusage(RUSAGE_SELF, &usage);
//                 double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
//                 printf("Elapsed time: %.2f%% seconds\n", elapsed_time);
//                 printf("Memory usage: %ld kilobytes\n", usage.ru_maxrss);
//             } else {
//                 printf("Error: Unknown algorithm '%s'.\n", algorithm);
//                 free(file_content);
//                 return 1;
//             }
//         } else if (strcmp(com_or_decom, "-d") == 0) {
//             // Decompression
//             if (strcmp(algorithm, "huffman") == 0) {
//                 printf("Decompressing %s using Huffman Coding...\n", input_file);
//                 decompress_huffman(input_file);
                
//             } else if (strcmp(algorithm, "arithmetic") == 0) {
//                 arithmetic_decompress(input_file);
//             } else if (strcmp(algorithm, "audio") == 0) {
//                 char input_file[256], output_file[256];
//                 printf("請輸入輸入檔案名稱: ");
//                 scanf("%s", input_file);
//                 printf("請輸入輸出檔案名稱: ");
//                 scanf("%s", output_file);
//                 decompress_audio(input_file, output_file);
//                 gettimeofday(&end, NULL);
//                 getrusage(RUSAGE_SELF, &usage);
//                 double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
//                 printf("Elapsed time: %.2f%% seconds\n", elapsed_time);
//                 printf("Memory usage: %ld kilobytes\n", usage.ru_maxrss);
//             } else {
//                 printf("Error: Unknown algorithm '%s'.\n", algorithm);
//                 free(file_content);
//                 return 1;
//             }
//         } else {
//             printf("Error: Unknown operation '%s'. Use -c for compression or -d for decompression.\n", com_or_decom);
//             free(file_content);
//             return 1;
//         }

//         // Free allocated memory
//         free(file_content);
//         printf("\n\n\n");

//     }
//     return 0;
// }
// todo list  1.把audio的壓縮解壓縮加進去 2.改一下make file讓audio compression時不用再多新增-lsndfile -lfftw3這樣子的指令 3.把main的部分改一下

//new version 3.0