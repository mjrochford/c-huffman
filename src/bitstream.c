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
typedef BitStream BitStreamWriter;
typedef BitStream BitStreamReader;

BitStreamReader *bitstream_reader_new(char *file_path)
{
    BitStreamReader *self = malloc(sizeof(*self));
    self->pending = 0;
    self->offset = 8;
    self->fd = open(file_path, O_RDONLY);
    if (self->fd < 0) {
        return NULL;
    }
    return self;
}

BitStreamWriter *bitstream_writer_new(char *file_path)
{
    BitStreamWriter *self = malloc(sizeof(*self));
    self->pending = 0;
    self->offset = 8;
    mode_t permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
    self->fd = open(file_path, O_WRONLY | O_CREAT, permissions);
    if (self->fd < 0) {
        return NULL;
    }
    return self;
}

void bitstream_flush(BitStream *bs)
{
    if (bs->offset == 8) {
        return;
    }

    write(bs->fd, &bs->pending, 1);
    bs->pending = 0;
    bs->offset = 8;
}

void bitstream_reader_close(BitStreamReader *self)
{
    close(self->fd);
    free(self);
}

void bitstream_writer_close(BitStreamWriter *self, bool flush)
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

void bitstream_write_bit(BitStreamWriter *bs, u_int8_t bit)
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

void bitstream_write_data(BitStreamWriter *bs, size_t data, u_int8_t offset)
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

int16_t bitstream_read_bit(BitStreamReader *bs)
{
    if (bs->offset == 8 || bs->offset == 0) {
        char c;
        ssize_t read_status = read(bs->fd, &c, 1);
        if (read_status == 0) {
            return -1;
        }
        bs->pending = c;
        bs->offset = 8;
    }

    bs->offset -= 1;
    return (bs->pending & (0x1 << bs->offset)) >> bs->offset;
}
