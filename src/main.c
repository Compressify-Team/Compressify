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

// Function to write a single bit to the output file using a buffer
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
        // Write '1' to indicate a leaf node and write the character
        writeBit(out_file, buffer, buffer_size, 1);
        fputc(root->val, out_file); // Write the character for leaf nodes
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
            code[depth] = '\0';
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

    // Encode the file content and write to the output file
    size_t encoded_len = 0;
    unsigned char* encoded_content = encode(forest[0], file_content, &encoded_len);
    if (!encoded_content) {
        perror("Encoding failed");
        fclose(out_file);
        return;
    }

    // Write the encoded content to the file (as a bit stream)
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
    printf("File content: %s\n", file_content);
    fclose(file);

    // Select the appropriate compression algorithm
    if (strcmp(algorithm, "huffman") == 0) {
        huffman_compress(file_content, file_size, input_file);
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