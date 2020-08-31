#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "b_heap.h"

#define SMPRINFT(str_name, format_str, ...)                                    \
    char *str_name;                                                            \
    do {                                                                       \
        size_t needed = snprintf(NULL, 0, format_str, __VA_ARGS__);            \
        str_name = malloc(needed);                                             \
        sprintf(str_name, format_str, __VA_ARGS__);                            \
    } while (0)

char *s_append(char *str, char *append)
{
    size_t needed = snprintf(NULL, 0, "%s%s", str, append);
    char *tree_file_path = malloc(needed);
    sprintf(tree_file_path, "%s%s", str, append);
    return tree_file_path;
}

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
    unsigned long data;
    unsigned short offset;
} HuffmanCode;

typedef struct BitStream {
    unsigned char pending;
    unsigned char offset;
    FILE *stream;
} BitStream;

int get_high_byte(HuffmanCode code)
{
    int shift = code.offset - 8;
    if (shift >= 0) {
        return (code.data & (0xFF << shift)) >> shift;
    }
    return -1;
}

void bitstream_flush(BitStream *bs)
{
    if (bs->offset == 0) {
        return;
    }

    fputc(bs->pending, bs->stream);
    bs->pending = 0;
    bs->offset = 0;
}

void bitstream_write_h_code(BitStream *bs, HuffmanCode code)
{
    if (code.offset < bs->offset) {
        // 00100000 == bs->pending
        //    ^     == bs->offset
        // 00000100 == code.data
        //      ^   == code.offset
        //      111 == 0xff >> (8 - code.offset) == bottom_bits_mask
        //    111    == code.offset - bs->offset

        bs->offset -= code.offset;
        size_t bottom_bits_mask = 0xff >> (8 - code.offset);
        bs->pending |= (code.data & bottom_bits_mask) << bs->offset;
        code.offset = 0;
    } else if (bs->offset > 0) {
        // 00101000 == bs->pending
        //      ^   == bs->offset
        // 00000100 == code.data
        //    ^     == code.offset
        //      111 == 0xff >> (8 - bs->offset) == top_bits_mask
        //    111   == code.offset - bs->offset

        code.offset -= bs->offset;
        size_t top_bits_mask = (0xff >> (8 - bs->offset)) << code.offset;
        bs->pending |= (code.data & top_bits_mask) >> code.offset;
        bitstream_flush(bs);
    }

    while (code.offset >= 8) {
        int high_byte = get_high_byte(code);
        assert(high_byte >= 0 && high_byte <= 255);
        fputc(high_byte, bs->stream);
        code.offset -= 8;
    }

    if (code.offset > 0) {
        bs->offset = 8 - code.offset;
        bs->pending = (code.data & (0xFF >> bs->offset)) << bs->offset;
    }
}

char *h_code_to_string(HuffmanCode self)
{
    char *string = malloc(sizeof(*string) * (self.offset + 1));
    for (int i = self.offset - 1; i >= 0; i--) {
        string[self.offset - 1 - i] = self.data & (1 << i) ? '1' : '0';
    }
    string[self.offset] = '\0';
    return string;
}

void h_tree_write(FILE *stream, HuffmanNode *root)
{
    fprintf(stream, "%c", root->symbol);
    if (root->left) {
        h_tree_write(stream, root->left);
    }
    if (root->right) {
        h_tree_write(stream, root->right);
    }
}

HuffmanCode h_tree_search(HuffmanNode *node, char c, HuffmanCode h_code)
{
    if (node->symbol == c) {
        return h_code;
    } else if (node->left) {
        HuffmanCode r_code = h_tree_search(
            node->left, c, (HuffmanCode){h_code.data << 1, h_code.offset + 1});
        if (r_code.offset != 0) {
            return r_code;
        }
    } else if (node->right) {
        HuffmanCode r_code = h_tree_search(
            node->right, c,
            (HuffmanCode){h_code.data << 1 | 1, h_code.offset + 1});
        if (r_code.offset != 0) {
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
        h_code.data = reverse_bits(h_code.data, h_code.offset);
        return h_code;
    } else if (leaf->parent->left == leaf) {
        HuffmanCode r_code = h_tree_bubble(
            leaf->parent, (HuffmanCode){h_code.data << 1, h_code.offset + 1});
        if (r_code.offset != 0) {
            return r_code;
        }
    } else if (leaf->parent->right == leaf) {
        HuffmanCode r_code =
            h_tree_bubble(leaf->parent, (HuffmanCode){h_code.data << 1 | 1,
                                                      h_code.offset + 1});
        if (r_code.offset != 0) {
            return r_code;
        }
    }

    return (HuffmanCode){0};
}

HuffmanNode *h_tree_from_heap(BHeap *heap)
{
    HuffmanNode *tree = b_heap_pop(heap);
    HuffmanNode *node = b_heap_pop(heap);
    while (node != NULL && tree != NULL) {
        HuffmanNode *branch = tree->freq > node->freq
                                  ? h_branch_new(tree, node)
                                  : h_branch_new(node, tree);

        b_heap_push(heap, branch);
        tree = b_heap_pop(heap);
        node = b_heap_pop(heap);
    }
    return tree;
}

void huff_write_tree_file(HuffmanNode *tree, char *input_path)
{
    SMPRINFT(tree_file_path, "%s.htree", input_path);
    FILE *tree_file = fopen(tree_file_path, "w");
    h_tree_write(tree_file, tree);
    free(tree_file_path);
    fclose(tree_file);
}

void huff_write_encoded_file(HuffmanNode **leaf_pointers, char *output_path,
                             FILE *in_file)
{
    FILE *out_file = fopen(output_path, "w");
    BitStream bs = {.stream = out_file};
    fseek(in_file, 0, SEEK_SET);
    char c;
    while ((c = fgetc(in_file)) != EOF) {
        HuffmanNode *leaf = leaf_pointers[(int)c];
        HuffmanCode h_code = h_tree_bubble(leaf, (HuffmanCode){0});
        bitstream_write_h_code(&bs, h_code);
    }
    bitstream_flush(&bs);

    fclose(out_file);
}

void huff_encode_file(char *input_path, char *output_path)
{
#define N_CHARACTERS 256
    size_t characters[N_CHARACTERS] = {0};
    HuffmanNode *leaf_pointers[N_CHARACTERS] = {0};

    FILE *in_file = fopen(input_path, "r");
    char c;
    while ((c = fgetc(in_file)) != EOF) {
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

    HuffmanNode *tree = h_tree_from_heap(heap);
    b_heap_free(heap);
    huff_write_tree_file(tree, input_path);
    huff_write_encoded_file(leaf_pointers, output_path, in_file);
    fclose(in_file);
}

HuffmanNode *h_tree_from_file(HuffmanNode *parent, FILE *tree_file)
{
    char c = fgetc(tree_file);
    if (c == EOF) {
        return NULL;
    }

    HuffmanNode *node = h_leaf_new(c, 0);
    node->left = NULL;
    node->right = NULL;
    node->parent = parent;
    if (c == '\0') {
        node->left = h_tree_from_file(node, tree_file);
    } else {
        node->right = h_tree_from_file(node, tree_file);
    }

    return node;
}

void huff_decode_file(char *input_path, char *output_path)
{
    SMPRINFT(tree_file_path, "%s.htree", input_path);
    FILE *tree_file = fopen(tree_file_path, "r");
    free(tree_file_path);
    tree_file_path = NULL;

    HuffmanNode *tree = h_tree_from_file(NULL, tree_file);
    fclose(tree_file);
}

int main()
{
    huff_encode_file("mobydick.txt", "mobydick.txt.huff");
    huff_decode_file("mobydick.txt", "mobydick.txt.2");
    // asdf
}
