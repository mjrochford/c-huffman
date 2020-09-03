#include "src/b_heap.h"
#include "src/bitstream.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct HuffmanNode {
    int symbol;
    size_t freq;
    struct HuffmanNode *parent;
    struct HuffmanNode *left;
    struct HuffmanNode *right;
} HuffmanNode;

typedef struct HuffmanCode {
    size_t data;
    u_int8_t offset;
} HuffmanCode;

int h_node_compare(void *hnode_a, void *hnode_b)
{
    int diff =
        (int)(((HuffmanNode *)hnode_a)->freq - ((HuffmanNode *)hnode_b)->freq);
    return -diff;
}

void h_node_print(void *node)
{
    HuffmanNode *h_node = node;
    printf("0x%.2X -> %lu", h_node->symbol, h_node->freq);
}

void h_node_free(void *self)
{
    HuffmanNode *me = self;
    if (me->right) {
        h_node_free(me->right);
    }
    if (me->left) {
        h_node_free(me->left);
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

HuffmanNode *h_leaf_new(int sym, size_t freq)
{
    HuffmanNode *self = malloc(sizeof(*self));
    self->symbol = sym;
    self->freq = freq;
    return self;
}

size_t h_tree_size(HuffmanNode *root)
{
    size_t count = 1;

    if (root->left) {
        count += h_tree_size(root->left);
    }
    if (root->right) {
        count += h_tree_size(root->right);
    }

    return count;
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

HuffmanCode h_tree_search(HuffmanNode *node, int c, HuffmanCode h_code)
{
    if (node->symbol == c) {
        return h_code;
    }
    if (node->left) {
        HuffmanCode r_code = h_tree_search(
            node->left, c, (HuffmanCode){h_code.data << 1, h_code.offset + 1});
        if (r_code.offset != 0) {
            return r_code;
        }
    }

    if (node->right) {
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
        if ((num & (1 << i))) {
            reverse_num |= 1 << ((n_bits - 1) - i);
        }
    }
    return reverse_num;
}

HuffmanCode h_tree_bubble(HuffmanNode *leaf, HuffmanCode h_code)
{
    if (leaf == NULL || leaf->parent == NULL) {
        h_code.data = reverse_bits(h_code.data, h_code.offset);
        return h_code;
    }

    if (leaf->parent->left == leaf) {
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

HuffmanNode *h_tree_from_file(HuffmanNode *parent, FILE *tree_file)
{
    int c = fgetc(tree_file);
    if (c == EOF) {
        return NULL;
    }

    HuffmanNode *node = h_leaf_new(c, 0);
    node->left = NULL;
    node->right = NULL;
    node->parent = parent;
    if (c == '\0') {
        node->left = h_tree_from_file(node, tree_file);
        node->right = h_tree_from_file(node, tree_file);
    }

    return node;
}

struct CharStream {
    const char *buffer;
    size_t index;
};

HuffmanNode *h_tree_from_stream(struct CharStream *stream, HuffmanNode *parent)
{
    char c = stream->buffer[stream->index];
    printf("%c %li\n", c, stream->index);
    stream->index++;

    HuffmanNode *node = h_leaf_new(c, 0);
    node->left = NULL;
    node->right = NULL;
    node->parent = parent;
    if (c == '\0') {
        node->left = h_tree_from_stream(stream, node);
        node->right = h_tree_from_stream(stream, node);
    }

    return node;
}

HuffmanNode *h_tree_from_buffer(const char *buffer)
{
    struct CharStream stream = {
        .buffer = buffer,
        .index = 0,
    };
    return h_tree_from_stream(&stream, NULL);
}

int h_tree_read_encoded_char(HuffmanNode *self, BitStreamReader *encoded_file)
{
    HuffmanNode *node = self;
    int16_t b = 0;
    while (b >= 0) {
        b = bitstream_read_bit(encoded_file);
        if (b == 0) {
            node = node->left;
        } else {
            node = node->right;
        }

        if (node->symbol != '\0') {
            return node->symbol;
        }
    }
    return EOF;
}
