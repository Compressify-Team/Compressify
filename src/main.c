#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "arith_cod.h"

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

void decompress_huffman(const char *file_content, size_t file_size, const char *input_file) {
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
                free(peek_node);
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
int huffman_compress(const char *file_content, size_t file_size, const char *input_file) {
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
                free(peek_node);
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

void arithmetic_compress(const char *file_content, size_t file_size, const char *input_file) {
    size_t input_size = strlen(file_content);

    size_t output_size = input_size * 2; // 預估輸出大小

    unsigned char* input = (unsigned char*)file_content;
    unsigned char* output = calloc(sizeof(unsigned char) * output_size, 1);

    // initialize the encoder state
    ac_state_t encoder_state;
    init_state(&encoder_state, 16);

    // build the probability table
    build_probability_table(&encoder_state, input, input_size);

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
    
    // 計算輸出大小
    size_t actual_output_size = calculate_output_size(output, output_size);

    fwrite(&input_size, sizeof(size_t), 1, out_file);
    printf("input_size: %zu\n", input_size);

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
    printf("output_size: %zu\n", actual_output_size);
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
    fclose(out_file);
    free(output);

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

    size_t output_size = input_size*2; // 計算實際輸出大小
    unsigned char* output = calloc(sizeof(unsigned char) * output_size,1);
    unsigned char* decomp = calloc(sizeof(unsigned char) * input_size,1); // 解碼後的大小應該與輸入大小相同

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
    // 解碼
    printf("Decoding...\n");

    decode_value(decomp, output, &encoder_state, input_size);

    // 輸出結果
    printf("Decoded string: %s\n", decomp);

    // 釋放記憶體
    free(output);
    free(decomp);
}

//-------------------------------------------arithmetic coding end-------------------------------------------


// Function to display input prompt
void show_prompt()
{
    printf("Please enter the decompression/compression algorithm and input file:\n");
    printf("Usage: -c/-d [algorithm] [input_file]\n");
}


// Main function
int main() 
{
    char com_or_decom[100];
    char algorithm[100];
    char input_file[100];

    // Display input prompt
    show_prompt();

    // Read user input
    if (scanf("%99s %99s %99s", com_or_decom, algorithm, input_file) != 3) {
        return 1;
    }


    if (strcmp(com_or_decom,"-c"))
    {
        printf("Compress or Decompress: compress\n");
    }
    else if (strcmp(com_or_decom,"-d"))
    {
        printf("Compress or Decompress: decompress\n");
    }
    else
    {
        printf("Error: Unknown operation '%s'. Use -c for compression or -d for decompression.\n", com_or_decom);
        return 1;
    }
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
    fclose(file);

    // Select the appropriate action based on user input
    if (strcmp(com_or_decom, "-c") == 0) {
        // Compression
        if (strcmp(algorithm, "huffman") == 0) {
            int compressed_size = huffman_compress(file_content, file_size, input_file);
            if(compressed_size != 0){
                //calculate the compression ratio
                printf("Compressed file size: %d bytes\n", compressed_size);
                double compression_ratio = (double)compressed_size / (double)file_size;
                printf("Compression ratio: %.2f\n", compression_ratio);
            }
        } else if (strcmp(algorithm, "arithmetic") == 0) {
            arithmetic_compress((unsigned char *)file_content, file_size, input_file);
        } else {
            printf("Error: Unknown algorithm '%s'.\n", algorithm);
            free(file_content);
            return 1;
        }
    } else if (strcmp(com_or_decom, "-d") == 0) {
        // Decompression
        if (strcmp(algorithm, "huffman") == 0) {
            // 假設有一個 huffman_decompress 函數
            printf("Decompressing %s using Huffman Coding...\n", input_file);
            decompress_huffman(file_content, file_size, input_file);
            
        } else if (strcmp(algorithm, "arithmetic") == 0) {
            arithmetic_decompress(input_file);
        } else {
            printf("Error: Unknown algorithm '%s'.\n", algorithm);
            free(file_content);
            return 1;
        }
    } else {
        printf("Error: Unknown operation '%s'. Use -c for compression or -d for decompression.\n", com_or_decom);
        free(file_content);
        return 1;
    }

    // Free allocated memory
    free(file_content);

    return 0;
}


