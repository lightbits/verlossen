#ifndef _buffer_h_
#define _buffer_h_

struct RingBuffer
{
    int read_index;
    int write_index;
    uint32 max_data_count;
    uint8 *data;

    void *Push(uint32 size);
    void *Pop(uint32 size);
};

RingBuffer MakeRingbuffer(uint8 *buffer, uint32 max_data_count);

#define RingPopStruct(rb, type) ((type*)rb.Push(sizeof(type)))
#define RingPushStruct(rb, type) ((type*)rb.Pop(sizeof(type)))

#endif
