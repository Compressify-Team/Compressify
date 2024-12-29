#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        printf("Serializing leaf node: %c\n", root->val); // Debug: Output leaf node value
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

// Function to print the Huffman codes
void printCode(Node* node, char* code, int depth) {
    if (!node->lchild && !node->rchild) {
        code[depth] = '\0';
        printf("'%c' (freq = %d) --> %s\n", node->val, freq[(unsigned char)node->val], code);
        return;
    }
    if (node->lchild) {
        code[depth] = '0';
        printCode(node->lchild, code, depth + 1);
    }
    if (node->rchild) {
        code[depth] = '1';
        printCode(node->rchild, code, depth + 1);
    }
}

// Function to find the Huffman code for a specific character
void findCode(Node* node, char* code, int depth, char target, char* result) {
    if (!node->lchild && !node->rchild) {
        if (node->val == target) {
            printf("Character '%c' found in tree.\n", target); // Debug: Output character found in tree
            if (depth == 0) {
                // Special case: if the tree has only one character, its code should be '0'
                code[depth] = '0';
                code[depth + 1] = '\0';
            }
            else {
                code[depth] = '\0';
            }
            printf("Code for character '%c': %s\n", target, code); // Debug: Output code for the character
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
    printf("Input: %s\n", input);
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
        printf("Character '%c' encoded as %s\n", input[i], result); // Debug: Output encoded character and its code
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
        printf("Deserialized leaf node: %c\n", val); // Debug: Output leaf node value
        return createNode(val, 0, NULL, NULL);
    } else {
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
        printf("Read bit: %d\n", bit); // Debug: Output bit value
        // Traverse the tree based on the bit
        if (bit == 0) {
            current = current->lchild;
        } else {
            current = current->rchild;
        }

        // If a leaf node is reached, write the character to the output file
        if (!current->lchild && !current->rchild) {
            printf("Decoded character: %c\n", current->val); // Debug: Output decoded character
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
        } else {
            Node* peek_node = stack[stack_size - 1];
            if (peek_node->rchild && last_visited != peek_node->rchild) {
                root_node = peek_node->rchild;
            } else {
                printf("Freeing node: %c\n", peek_node->val); // Debug: Output node being freed
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
void huffman_compress(const char *file_content, size_t file_size, const char *input_file) {
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
        return;
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
        return;
    }
    // Print encoded content in binary form
    printf("Encoded content in binary form:\n");
    for (size_t i = 0; i < encoded_len; i++) {
        for (int j = 7; j >= 0; j--) {
            printf("%d", (encoded_content[i] >> j) & 1);
        }
        printf(" ");
    }
    printf("\n");
    // print encoded_len 
    printf("Encoded length: %zu\n", encoded_len);
    // Write the encoded content to the file
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

    free(encoded_content);
    fclose(out_file);
}

// Function to display usage information
void show_help() {
    printf("Usage: compress [algorithm] input_file\n");
}

// Main function
int main(int argc, char *argv[]) {
    // Check if the correct number of arguments is provided
    if (argc != 3) {
        show_help();
        return 1;
    }

    const char *algorithm = argv[1];
    const char *input_file = argv[2];

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
    char *file_content = (char *)malloc(file_size);
    if (file_content == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return 1;
    }

    // Read the file content
    fread(file_content, 1, file_size, file);
    fclose(file);

    // Select the appropriate compression algorithm
    if (strcmp(algorithm, "huffman") == 0) {
        huffman_compress(file_content, file_size, input_file);
    }
    else if(strcmp(algorithm, "huffman-de") == 0) {
        printf("Decompressing %s using Huffman coding...\n", input_file);
        decompress_huffman(file_content, file_size, input_file);
    }
    else {
        printf("Error: Unknown algorithm '%s'.\n", algorithm);
        show_help();
        free(file_content);
        return 1;
    }

    // Free allocated memory
    free(file_content);

    return 0;
}