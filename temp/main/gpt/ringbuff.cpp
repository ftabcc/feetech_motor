size_t ByteRingBuffer::write(const uint8_t *data, size_t len)
{
    if (data == nullptr || len == 0)
    {return 0;}
    size_t written = 0;
    while (written < len)
    {
        size_t next = (write_idx + 1) % CAPACITY;
        if (next == read_idx)
        {break;} // Full
        buffer[write_idx] = data[written];
        write_idx = next;
        written++;
    }
    return written;
}

bool ByteRingBuffer::read(uint8_t &byte)
{
    if (read_idx == write_idx)
    {return false;} // Empty
    byte = buffer[read_idx];
    read_idx = (read_idx + 1) % CAPACITY;
    return true;
}

size_t ByteRingBuffer::available() const
{
    if (write_idx >= read_idx)
    {return write_idx - read_idx;}
    return CAPACITY - read_idx + write_idx;
}

bool ByteRingBuffer::empty() const
{return read_idx == write_idx;}

bool ByteRingBuffer::full() const
{return ((write_idx + 1) % CAPACITY) == read_idx;}

void ByteRingBuffer::clear()
{read_idx = write_idx;}