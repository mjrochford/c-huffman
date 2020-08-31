#pragma once
#include <stdbool.h>
#include <stdlib.h>

typedef struct BitStream_s BitStream;
BitStream *bitstream_read_new(char *file_path);
BitStream *bitstream_write_new(char *file_path);

void bitstream_flush(BitStream *bs);
void bitstream_close(BitStream *self, bool flush);
void bitstream_write_bit(BitStream *bs, u_int8_t bit);
void bitstream_write_data(BitStream *bs, size_t data, u_int8_t offset);
