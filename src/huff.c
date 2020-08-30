#include <stdbool.h>
#include <stdio.h>

#include "b_heap.h"

typedef struct HuffmanNode {
    char symbol;
    size_t freq;
    struct HuffmanNode *parent;
    struct HuffmanNode *left;
    struct HuffmanNode *right;
} HuffmanNode;

void h_node_free(void *self)
{
    HuffmanNode *me = self;
    if (me->right) {
        h_node_free(me->right);
    }
    if (me->left) {
        h_node_free(me->right);
    }
    free(me);
}

HuffmanNode *h_branch_new(HuffmanNode *leaf_l, HuffmanNode *leaf_r)
{
    HuffmanNode *self = malloc(sizeof(*self));
    self->freq = leaf_l->freq + leaf_r->freq;
    self->symbol = '\0';
    self->parent = NULL;
    self->left = leaf_l;
    self->right = leaf_r;
    self->left->parent = self;
    self->right->parent = self;
    return self;
}

HuffmanNode *h_leaf_new(char sym, size_t freq)
{
    HuffmanNode *self = malloc(sizeof(*self));
    self->symbol = sym;
    self->freq = freq;
    return self;
}

int h_node_compare(void *hnode_a, void *hnode_b)
{
    int diff = ((HuffmanNode *)hnode_a)->freq - ((HuffmanNode *)hnode_b)->freq;
    return -diff;
}

void h_node_print(void *node)
{
    HuffmanNode *h_node = node;
    printf("0x%.2X -> %li", h_node->symbol, h_node->freq);
}

typedef struct HuffmanCode {
    unsigned long code;
    unsigned short offset;
} HuffmanCode;

char *h_code_to_string(HuffmanCode self)
{
    char *string = malloc(sizeof(*string) * (self.offset + 1));
    for (int i = self.offset - 1; i >= 0; i--) {
        string[self.offset - 1 - i] = self.code & (1 << i) ? '1' : '0';
    }
    string[self.offset] = '\0';
    return string;
}

void h_tree_write(FILE *stream, HuffmanNode *root, HuffmanCode h_code)
{
    if (root->symbol != '\0') {
        char *code_str = h_code_to_string(h_code);
        fprintf(stream, "%.2X:%s\n", root->symbol, code_str);
        free(code_str);
    }
    if (root->left) {
        h_tree_write(stream, root->left, (HuffmanCode){h_code.code << 1, h_code.offset + 1});
    }
    if (root->right) {
        h_tree_write(stream, root->right, (HuffmanCode){h_code.code << 1 | 1, h_code.offset + 1});
    }
}

HuffmanCode h_tree_search(HuffmanNode *node, char c, HuffmanCode h_code)
{
    if (node->symbol == c) {
        return h_code;
    } else if (node->left) {
        HuffmanCode r_code =
            h_tree_search(node->left, c, (HuffmanCode){h_code.code << 1, h_code.offset + 1});
        if (r_code.code != 0 && r_code.offset != 0) {
            return r_code;
        }
    } else if (node->right) {
        HuffmanCode r_code =
            h_tree_search(node->right, c, (HuffmanCode){h_code.code << 1 | 1, h_code.offset + 1});
        if (r_code.code != 0 && r_code.offset != 0) {
            return r_code;
        }
    }

    return (HuffmanCode){0};
}

size_t reverse_bits(size_t num, size_t n_bits)
{
    size_t reverse_num = 0;
    for (size_t i = 0; i < n_bits; i++) {
        if ((num & (1 << i)))
            reverse_num |= 1 << ((n_bits - 1) - i);
    }
    return reverse_num;
}

HuffmanCode h_tree_bubble(HuffmanNode *leaf, HuffmanCode h_code)
{
    if (leaf == NULL || leaf->parent == NULL) {
        h_code.code = reverse_bits(h_code.code, h_code.offset);
        return h_code;
    } else if (leaf->parent->left == leaf) {
        HuffmanCode r_code =
            h_tree_bubble(leaf->parent, (HuffmanCode){h_code.code << 1, h_code.offset + 1});
        if (r_code.code != 0 && r_code.offset != 0) {
            return r_code;
        }
    } else if (leaf->parent->right == leaf) {
        HuffmanCode r_code =
            h_tree_bubble(leaf->parent, (HuffmanCode){h_code.code << 1 | 1, h_code.offset + 1});
        if (r_code.code != 0 && r_code.offset != 0) {
            return r_code;
        }
    }

    return (HuffmanCode){0};
}

HuffmanNode *h_tree_from(BHeap *heap)
{
    HuffmanNode *tree = b_heap_pop(heap);
    HuffmanNode *node = b_heap_pop(heap);
    while (node != NULL && tree != NULL) {
        HuffmanNode *branch =
            tree->freq > node->freq ? h_branch_new(tree, node) : h_branch_new(node, tree);

        b_heap_push(heap, branch);
        tree = b_heap_pop(heap);
        node = b_heap_pop(heap);
    }
    return tree;
}

void huff_encode_file(char *input_path, char *output_path)
{
#define N_CHARACTERS 256
    FILE *in_file = fopen(input_path, "r");
    size_t characters[N_CHARACTERS] = {0};
    HuffmanNode *leaf_pointers[N_CHARACTERS] = {0};

    while (!feof(in_file)) {
        char c = fgetc(in_file);
        characters[(size_t)c] += 1;
    }

    BHeap *heap = b_heap_new_full(h_node_compare, h_node_free);
    for (size_t i = 0; i < N_CHARACTERS; i++) {
        if (characters[i] > 0) {
            HuffmanNode *leaf = h_leaf_new((char)i, characters[i]);
            leaf_pointers[i] = leaf;
            b_heap_push(heap, leaf);
        }
    }

    HuffmanNode *tree = h_tree_from(heap);
    size_t needed = snprintf(NULL, 0, "%s.htree", input_path);
    char *tree_file_path = malloc(needed);
    sprintf(tree_file_path, "%s.htree", input_path);
    FILE *tree_file = fopen(tree_file_path, "w");
    h_tree_write(tree_file, tree, (HuffmanCode){0});
    free(tree_file_path);
    fclose(tree_file);

    FILE *out_file = fopen(output_path, "w");
    fseek(in_file, 0, SEEK_SET);
    while (!feof(in_file)) {
        char c = fgetc(in_file);
        HuffmanNode *leaf = leaf_pointers[(int)c];
        HuffmanCode h_code = h_tree_bubble(leaf, (HuffmanCode){0});
        // char *code_str = h_code_to_string(h_code);
        // fprintf(stdout, "%.2X:%s\n", c, code_str);
        // free(code_str);
        // write code to output
    }

    fclose(out_file);
    fclose(in_file);
    b_heap_free(heap);
}

int main()
{
    huff_encode_file("mobydick.txt", "mobydick.txt.huff");
    // asdf
}
