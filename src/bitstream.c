#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct BitStream {
    u_int8_t pending;
    u_int8_t offset;
    int fd;
} BitStream;

BitStream *bitstream_read_new(char *file_path)
{
    BitStream *self = malloc(sizeof(*self));
    self->pending = 0;
    self->offset = 8;
    self->fd = open(file_path, O_RDONLY);
    if (self->fd < 0) {
        return NULL;
    }
    return self;
}

BitStream *bitstream_write_new(char *file_path)
{
    BitStream *self = malloc(sizeof(*self));
    self->pending = 0;
    self->offset = 8;
    self->fd = open(file_path, O_WRONLY | O_CREAT);
    if (self->fd < 0) {
        return NULL;
    }
    return self;
}

void bitstream_flush(BitStream *bs)
{
    if (bs->offset == 0) {
        return;
    }

    write(bs->fd, &bs->pending, 1);
    bs->pending = 0;
    bs->offset = 8;
}

void bitstream_close(BitStream *self, bool flush)
{
    if (flush) {
        bitstream_flush(self);
    }
    close(self->fd);
    free(self);
}

int get_high_byte(size_t data, size_t offset)
{
    int shift = offset - 8;
    if (shift >= 0) {
        return (data & (0xFF << shift)) >> shift;
    }
    return -1;
}

int get_bot_bits(size_t data, size_t n_bits)
{
    assert(n_bits < 8);
    //...000100 == data
    //      ^   == offset
    // 11111111 == 0xff
    //      111 == 0xff >> shift == bottom_bits_mask
    int shift = 8 - n_bits;
    size_t bottom_bits_mask = (data & (0xFF >> shift));
    return bottom_bits_mask & data;
}

int get_top_bits(size_t data, size_t n_bits, size_t offset)
{
    assert(offset > (8 - n_bits));
    // 00000100 == data
    //    ^     == offset
    // 11111111 == 0xff
    //      111 == 0xff >> (8 - n_bits) == mask
    //    111   == mask << (offset - n_bits) = top_bits_mask
    //      001 == (top_bits_mask & data) >> offset
    offset -= n_bits;
    size_t top_bits_mask = (0xff >> (8 - n_bits)) << offset;
    return (top_bits_mask & data) >> offset;
}

void bitstream_write_bit(BitStream *bs, u_int8_t bit)
{
    // 00000001 bit
    // xxxx0000 bs->pending
    //     ^    offset
    // 00001000 bit << (bs->offset - 1)
    bs->pending |= bit << (bs->offset - 1);
    bs->offset -= 1;

    if (bs->offset == 0) {
        bitstream_flush(bs);
    }
}

void bitstream_write_data(BitStream *bs, size_t data, u_int8_t offset)
{
    if (offset < bs->offset) {
        bs->offset -= offset;
        bs->pending |= get_bot_bits(data, offset) << bs->offset;
        offset = 0;
    } else if (bs->offset < 8 && bs->offset > 0) {
        bs->pending |= get_top_bits(data, bs->offset, offset);
        offset -= bs->offset;
        bitstream_flush(bs);
    }

    while (offset >= 8) {
        int high_byte = get_high_byte(data, offset);
        assert(high_byte >= 0 && high_byte <= 255);
        u_int8_t buff = high_byte;
        write(bs->fd, &buff, 1);
        offset -= 8;
    }

    if (offset > 0) {
        bs->offset = 8 - offset;
        bs->pending = (data & (0xFF >> bs->offset)) << bs->offset;
    }
}
