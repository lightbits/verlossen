#ifndef _buffer_h_
#define _buffer_h_

struct RingBuffer
{
    int read_index;
    int write_index;
    uint32 max_data_count;
    uint8 *data;

    void *PushSize(uint32 size);
    void *PopSize(uint32 size);
};

RingBuffer MakeRingbuffer(uint8 *buffer, uint32 max_data_count);

#define PopStruct(rb, type) ((type*)rb.PopSize(sizeof(type)))
#define PushStruct(rb, type) ((type*)rb.PushSize(sizeof(type)))

#endif
