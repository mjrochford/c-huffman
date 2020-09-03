#include "src/bitstream.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct HuffmanNode_s HuffmanNode;
typedef struct HuffmanCode {
    size_t data;
    u_int8_t offset;
} HuffmanCode;

int h_node_compare(void *hnode_a, void *hnode_b);
void h_node_print(void *node);
void h_node_free(void *self);
HuffmanNode *h_branch_new(HuffmanNode *leaf_l, HuffmanNode *leaf_r);
HuffmanNode *h_leaf_new(int sym, size_t freq);
size_t h_tree_size(HuffmanNode *root);

void h_tree_write(FILE *stream, HuffmanNode *root);
HuffmanCode h_tree_search(HuffmanNode *node, int c, HuffmanCode h_code);
size_t reverse_bits(size_t num, size_t n_bits);
HuffmanCode h_tree_bubble(HuffmanNode *leaf, HuffmanCode h_code);
HuffmanNode *h_tree_from_file(HuffmanNode *parent, FILE *tree_file);
HuffmanNode *h_tree_from_buffer(char buffer[]);
int h_tree_read_encoded_char(HuffmanNode *self, BitStreamReader *bs);
