#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "b_heap.h"
#include "bitstream.h"
#include "h_tree.h"

#define SMPRINFT(str_name, format_str, ...)                                    \
    char *str_name;                                                            \
    do {                                                                       \
        size_t needed = snprintf(NULL, 0, format_str, __VA_ARGS__);            \
        str_name = malloc(needed);                                             \
        sprintf(str_name, format_str, __VA_ARGS__);                            \
    } while (0)

HuffmanNode *h_tree_from_heap(BHeap *heap)
{
    HuffmanNode *tree = b_heap_pop(heap);
    HuffmanNode *node = b_heap_pop(heap);
    while (node != NULL && tree != NULL) {
        HuffmanNode *branch = h_node_compare(tree, node) < 0
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
    free(tree_file_path);

    h_tree_write(tree_file, tree);

    fclose(tree_file);
}

void huff_write_encoded_file(HuffmanNode **leaf_pointers, char *output_path,
                             FILE *in_file)
{
    BitStreamWriter *bs = bitstream_writer_new(output_path);
    fseek(in_file, 0, SEEK_SET);
    int c;
    while ((c = fgetc(in_file)) != EOF) {
        HuffmanNode *leaf = leaf_pointers[(int)c];
        HuffmanCode h_code = h_tree_bubble(leaf, (HuffmanCode){0});
        bitstream_write_data(bs, h_code.data, h_code.offset);
    }
    bitstream_writer_close(bs, true);
}

void huff_encode_file(char *input_path, char *output_path)
{
#define N_CHARACTERS 256
    size_t characters[N_CHARACTERS] = {0};
    HuffmanNode *leaf_pointers[N_CHARACTERS] = {0};

    FILE *in_file = fopen(input_path, "r");
    int c;
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
    h_node_free(tree);
    fclose(in_file);
}

void huff_decode_file(char *input_path, char *output_path)
{
    SMPRINFT(tree_file_path, "%s.htree", input_path);
    FILE *tree_file = fopen(tree_file_path, "r");
    free(tree_file_path);
    tree_file_path = NULL;

    HuffmanNode *tree = h_tree_from_file(NULL, tree_file);
    fclose(tree_file);

    SMPRINFT(encoded_file_path, "%s.huff", input_path);
    BitStreamReader *encoded_file = bitstream_reader_new(encoded_file_path);
    FILE *out_file = fopen(output_path, "w");
    char c;
    while (EOF != (c = h_tree_read_encoded_char(tree, encoded_file))) {
        fputc(c, out_file);
    }

    h_node_free(tree);
    bitstream_reader_close(encoded_file);
    fclose(out_file);
}

int main()
{
    // huff_encode_file("mobydick.txt", "mobydick.txt.huff");
    // huff_decode_file("mobydick.txt", "mobydick.2.txt");

    huff_encode_file("test.txt", "test.txt.huff");
    huff_decode_file("test.txt", "test.2.txt");
}
