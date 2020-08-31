#pragma once
#include <stdbool.h>
#include <stdlib.h>

typedef struct BitStream_s BitStreamWriter;
typedef struct BitStream_s BitStreamReader;
BitStreamReader *bitstream_reader_new(char *file_path);
BitStreamWriter *bitstream_writer_new(char *file_path);
void bitstream_reader_close(BitStreamReader *self);
void bitstream_writer_close(BitStreamWriter *self, bool flush);
void bitstream_flush(BitStreamWriter *bs);

void bitstream_write_bit(BitStreamWriter *bs, u_int8_t bit);
void bitstream_write_data(BitStreamWriter *bs, size_t data, u_int8_t offset);
int16_t bitstream_read_bit(BitStreamReader *bs);
