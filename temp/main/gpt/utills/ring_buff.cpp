#include "ring_buff.h"

RingBuffer::RingBuffer(size_t size): buffer(nullptr), capacity(size)
{
    if (capacity == 0)
    {return;}
    buffer = new uint8_t[capacity]{};
}

RingBuffer::~RingBuffer()
{delete[] buffer;}

size_t RingBuffer::write(const uint8_t *data, size_t len)
{
    if (buffer == nullptr || data == nullptr || len == 0)
    {return 0;}
    size_t written = 0;
    while (written < len && count < capacity)
    {
        buffer[write_idx] = data[written];
        write_idx = (write_idx + 1) % capacity;

        count++;
        written++;
    }
    return written;
}

bool RingBuffer::read(uint8_t &byte)
{
    if (count == 0)
    {return false;}
    byte = buffer[read_idx];
    read_idx = (read_idx + 1) % capacity;
    count--;
    return true;
}

size_t RingBuffer::available() const
{return count;}

size_t RingBuffer::free_space() const
{return capacity - count;}

bool RingBuffer::empty() const
{return count == 0;}

bool RingBuffer::full() const
{return count == capacity;}

void RingBuffer::clear()
{
    write_idx = 0;
    read_idx = 0;
    count = 0;
}