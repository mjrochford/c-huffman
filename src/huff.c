#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "b_heap.h"
#include "bitstream.h"
#include "h_tree.h"

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

void huff_write_tree_to_file(HuffmanNode *tree, char *out_file_path)
{
    FILE *out_file = fopen(out_file_path, "w");
    size_t n_nodes = h_tree_size(tree);
    fwrite(&n_nodes, sizeof(n_nodes), 1, out_file);
    h_tree_write(out_file, tree);
    fclose(out_file);
}

void huff_write_codes_to_file(HuffmanNode **leaf_pointers, char *out_path,
                              char *in_path)
{
    FILE *in_file = fopen(in_path, "r");
    BitStreamWriter *bs = bitstream_writer_new(out_path);
    int c;
    while ((c = fgetc(in_file)) != EOF) {
        HuffmanNode *leaf = leaf_pointers[(int)c];
        HuffmanCode h_code = h_tree_bubble(leaf, (HuffmanCode){0});
        bitstream_write_data(bs, h_code.data, h_code.offset);
    }
    fclose(in_file);
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
    fclose(in_file);

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

    huff_write_tree_to_file(tree, output_path);
    huff_write_codes_to_file(leaf_pointers, output_path, input_path);

    h_node_free(tree);
}

void huff_decode_file(char *encoded_path, char *decoded_path)
{
    FILE *encoded_file = fopen(encoded_path, "r");
    size_t n_nodes;
    size_t encoded_file_data_offset =
        sizeof(n_nodes) * fread(&n_nodes, sizeof(n_nodes), 1, encoded_file);

    char tree_buffer[n_nodes];
    encoded_file_data_offset +=
        fread(tree_buffer, sizeof(char), n_nodes, encoded_file);
    fclose(encoded_file);

    HuffmanNode *tree = h_tree_from_buffer(tree_buffer, n_nodes);

    BitStreamReader *encoded_file_stream =
        bitstream_reader_new_offset(encoded_path, encoded_file_data_offset);
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
    huff_encode_file("mobydick.txt", "mobydick.txt.huff");
    huff_decode_file("mobydick.txt.huff", "mobydick.2.txt");
#if 0
    huff_encode_file("test.txt", "test.txt.huff");
    huff_decode_file("test.txt.huff", "test.2.txt");
#endif
}
