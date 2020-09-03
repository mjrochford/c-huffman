#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "b_heap.h"
#include "bitstream.h"
#include "h_tree.h"

HuffmanNode *huff_tree_from_heap(BHeap *heap)
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

void huff_write_tree_to_file(HuffmanNode *tree, char *out_file_path)
{
    FILE *out_file = fopen(out_file_path, "w");
    h_tree_write(out_file, tree);
    fclose(out_file);
}

void huff_write_codes_to_file(HuffmanCode codes[], char *out_path,
                              char *in_path)
{
    FILE *in_file = fopen(in_path, "r");
    BitStreamWriter *bs = bitstream_writer_new(out_path);
    int c;
    while ((c = fgetc(in_file)) != EOF) {
        HuffmanCode h_code = codes[c];
        bitstream_write_data(bs, h_code.data, h_code.offset);
    }
    fclose(in_file);
    bitstream_writer_close(bs, true);
}

#define N_CHARACTERS 256
BHeap *huff_create_node_heap(char *input_path, int *characters,
                             HuffmanNode **leafs)
{
    FILE *in_file = fopen(input_path, "r");
    int c;
    while ((c = fgetc(in_file)) != EOF) {
        characters[(size_t)c] += 1;
    }
    fclose(in_file);

    BHeap *heap = b_heap_new_full(h_node_compare, h_node_free);
    for (size_t i = 0; i < N_CHARACTERS; i++) {
        if (characters[i] > 0) {
            HuffmanNode *leaf = h_leaf_new((char)i, characters[i]);
            leafs[i] = leaf;
            b_heap_push(heap, leaf);
        }
    }
    return heap;
}

void huff_encode_file(char *input_path, char *output_path)
{
    int characters[N_CHARACTERS] = {0};
    HuffmanNode *leaf_pointers[N_CHARACTERS] = {0};

    BHeap *heap = huff_create_node_heap(input_path, characters, leaf_pointers);
    HuffmanNode *tree = huff_tree_from_heap(heap);
    b_heap_free(heap);

    HuffmanCode codes[N_CHARACTERS] = {0};
    for (int i = 0; i < N_CHARACTERS; i++) {
        codes[i] = h_tree_bubble(leaf_pointers[i], (HuffmanCode){0});
    }

    huff_write_tree_to_file(tree, output_path);
    huff_write_codes_to_file(codes, output_path, input_path);

    h_node_free(tree);
}

void huff_decode_file(char *encoded_path, char *decoded_path)
{
    FILE *encoded_file = fopen(encoded_path, "r");
    HuffmanNode *tree = h_tree_from_file(NULL, encoded_file);
    size_t n_nodes = h_tree_size(tree);

    BitStreamReader *encoded_file_stream =
        bitstream_reader_new_offset(encoded_path, n_nodes);
    FILE *out_file = fopen(decoded_path, "w");
    int c;
    while (EOF != (c = h_tree_read_encoded_char(tree, encoded_file_stream))) {
        fputc(c, out_file);
    }
    h_node_free(tree);
    bitstream_reader_close(encoded_file_stream);
    fclose(out_file);
}

int main()
{
#if 1
    huff_encode_file("mobydick.txt", "mobydick.txt.huff");
    huff_decode_file("mobydick.txt.huff", "mobydick.2.txt");
#endif
#if 0
    huff_encode_file("test.txt", "test.txt.huff");
    huff_decode_file("test.txt.huff", "test.2.txt");
#endif
}
