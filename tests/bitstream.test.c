#include "../src/bitstream.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void bitstream_test_write_bit(char *test_file_path)
{
    BitStream *bs = bitstream_write_new(test_file_path);
    bitstream_write_bit(bs, 0x1);
    bitstream_write_bit(bs, 0x0);
    bitstream_write_bit(bs, 0x1);
    bitstream_close(bs, true);

    FILE *test_file = fopen(test_file_path, "r");
    u_int8_t c = fgetc(test_file);
    assert(c == 0xA0); // 0xA0 == 0b10100000
    fclose(test_file);

    bs = bitstream_write_new(test_file_path);
    bitstream_write_bit(bs, 0x1);
    bitstream_write_bit(bs, 0x0);
    bitstream_write_bit(bs, 0x1);
    bitstream_write_bit(bs, 0x1);
    bitstream_write_bit(bs, 0x1);
    bitstream_write_bit(bs, 0x1);
    bitstream_write_bit(bs, 0x0);
    bitstream_write_bit(bs, 0x1);
    bitstream_close(bs, true);

    test_file = fopen(test_file_path, "r");
    c = fgetc(test_file);
    assert(c == 0xBD); // 0xBD = 0b10111101
    fclose(test_file);
}

void bitstream_test_write_data(char *test_file_path)
{
    BitStream *bs = bitstream_write_new(test_file_path);
    bitstream_write_data(bs, 0x555, 16); // 0x555 = 0b10101010101
    bitstream_close(bs, true);

    FILE *test_file = fopen(test_file_path, "r");
    u_int8_t c = fgetc(test_file);
    assert(c == 0x5); // 0x5 = 0b00000101
    c = fgetc(test_file);
    assert(c == 0x55); // 0x555 = 0b01010101
    fclose(test_file);

    bs = bitstream_write_new(test_file_path);
    bitstream_write_data(bs, 0x2796, 18); // 0x2796 = 0b000010011110010110
    bitstream_close(bs, true);

    test_file = fopen(test_file_path, "r"); // 00001001 11100101 10000000
    c = fgetc(test_file);
    assert(c == 0x09); //  0x09 = 0b00001001
    c = fgetc(test_file);
    assert(c == 0xE5); // 0xE5 = 0b11100101
    c = fgetc(test_file);
    assert(c == 0x80); // 0x80 = 0b10000000
    fclose(test_file);
}

int main()
{
    char *test_file_path = "bitstream-test.bin";
    bitstream_test_write_bit(test_file_path);
    bitstream_test_write_data(test_file_path);
}
