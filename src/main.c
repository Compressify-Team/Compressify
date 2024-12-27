#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHAR 256

typedef struct Node {
    char val;
    int freq;
    struct Node* lchild;
    struct Node* rchild;
} 
Node;

// Frequency table and forest array
int freq[MAX_CHAR] = {0};
Node* forest[MAX_CHAR];
int forest_size = 0;

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

char* encode(Node* root, const char* input) {
    char code[256] = {0};
    char result[256]= {0};
    char* encoded = (char*) malloc(256);
    if (encoded) {
        encoded[0] = '\0'; // 確保是空字串
    }
    for (int i = 0; input[i] != '\0'; i++) {
        findCode(root, code, 0, input[i], result);
        encoded = strcat(encoded, result);
        printf("%s", result);
    }
    printf("\n");
    return encoded;
}

void decode(Node* root, const char* encodedString) {
    Node* currentNode = root;
    for (int i = 0; encodedString[i] != '\0'; i++) {
        if (encodedString[i] == '0') {
            currentNode = currentNode->lchild;
        }
        else {
            currentNode = currentNode->rchild;
        }
        if (!currentNode->lchild && !currentNode->rchild) {
            printf("%c", currentNode->val);
            currentNode = root;
        }
    }
    printf("\n");
}

// Function declarations for compression algorithms
void huffman_compress(const char *file_content, size_t file_size, const char *input_file) 
{
    int c;
    char encoded[256] = {0};
    char output_file[256] = {0};
    snprintf(output_file, sizeof(output_file), "%s.huf", input_file);
    printf("Compressing %s to %s using Huffman Coding...\n", input_file, output_file);
    //count the frequency of each character
    for(int i = 0; i < MAX_CHAR; i++) {
        int c = file_content[i];
        freq[c]++;
    }
    // Create nodes for each character with non-zero frequency
    for (int i = 0; i < MAX_CHAR; i++) {
        if (freq[i] > 0) {
            forest[forest_size++] = createNode(i, freq[i], NULL, NULL);
        }
    }
    // Build the Huffman tree
    while (forest_size > 1) {
        // Sort the forest by frequency
        qsort(forest, forest_size, sizeof(Node*), compare);

        // Extract the two nodes with the smallest frequency
        Node* left = forest[0];
        Node* right = forest[1];

        // Create a new parent node combining these two
        Node* parent = createNode(-1, left->freq + right->freq, left, right);

        // Insert the new parent node back into the forest
        forest[0] = parent; // Place parent at index 0

        // Shift remaining elements in the forest down by one position
        for (int i = 1; i < forest_size - 1; i++) {
            forest[i] = forest[i + 1];
        }
        
        // Decrease the size of the forest
        forest_size--;
    }

    // Print the Huffman codes
    char code[MAX_CHAR];
    //printCode(forest[0], code, 0);
    strcpy(encoded, encode(forest[0], file_content));
    printf("Decoded: \n");
    decode(forest[0], encoded);


    // Free memory (post-order travesrsal to free nodes)
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

    // Open the output file for writing
    FILE *out_file = fopen(output_file, "wb");
    if (out_file == NULL) {
        perror("Error opening output file");
        return;
    }
    printf("Encoded: %s\n", encoded);
    // TODO: Add Huffman Coding implementation here
    fputs(encoded, out_file);
    // Close the output file
    fclose(out_file);
}

void arithmetic_compress(const char *file_content, size_t file_size, const char *input_file) 
{
    char output_file[256];
    snprintf(output_file, sizeof(output_file), "%s.arc", input_file);
    printf("Compressing %s to %s using Arithmetic Coding...\n", input_file, output_file);

    // Open the output file for writing
    FILE *out_file = fopen(output_file, "wb");
    if (out_file == NULL) {
        perror("Error opening output file");
        return;
    }

    // TODO: Add Arithmetic Coding implementation here
    fprintf(out_file, "Example compressed content (Arithmetic)\n");

    // Close the output file
    fclose(out_file);
}

// Function to display usage information
void show_help() 
{
    printf("Usage: compress [algorithm] input_file\n");
}

// Main function
int main(int argc, char *argv[]) 
{
    // Check if the correct number of arguments is provided
    if (argc != 3) 
    {
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
        show_help();
        free(file_content);
        return 1;
    }


    // Free allocated memory
    free(file_content);

    return 0;
}
