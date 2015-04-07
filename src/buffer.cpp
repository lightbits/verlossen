#include "buffer.h"

int
RingBuffer::GetElementCount()
{
    if (write_index < 0 && read_index > 0) // We are full
        return max_data_count;
    if (write_index < 0 && read_index < 0)
        return 0;
    /*
    R-------W------
    |-------|
      count

    --W--------R---
    --|        |---
    nt          cou
    */
    if (read_index <= write_index)
        return write_index - read_index;
    else
        return max_data_count - read_index + write_index;
}

void *
RingBuffer::Push(uint32 size)
{
    if (write_index == -1)
        return 0;
    if (read_index == -1)
        read_index = write_index;
    void *result = data + write_index * size;
    write_index = (write_index + 1) % max_data_count;
    if (write_index == read_index)
        write_index = -1;
    return result;
}

void *
RingBuffer::Pop(uint32 size)
{
    if (read_index == -1)
        return 0;
    if (write_index == -1)
        write_index = read_index;
    void *result = data + read_index * size;
    read_index = (read_index + 1) % max_data_count;
    if (read_index == write_index)
        read_index = -1;
    return result;
}

RingBuffer
MakeRingBuffer(uint8 *buffer, uint32 max_data_count)
{
    RingBuffer result = {};
    result.data = buffer;
    result.max_data_count = max_data_count;
    result.read_index = -1;
    result.write_index = 0;
    return result;
}
