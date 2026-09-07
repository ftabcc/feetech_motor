
RingBuffer::RingBuffer()
{
    write_idx = 0;
    read_idx = 0;
    count = 0;
}

bool RingBuffer::write(uint8_t data)
{
    if (count >= RING_BUFFER_SIZE)
    {return false;}

    buffer[write_idx] = data;
    write_idx++;
    if (write_idx >= RING_BUFFER_SIZE)
    {write_idx = 0;}
    count++;
    return true;
}

size_t RingBuffer::write(const uint8_t* data, size_t len)
{
    size_t written = 0;
    while (written < len)
    {
        if (!write(data[written]))
        {break;}
        written++;
    }
    return written;
}

bool RingBuffer::read(uint8_t& data)
{
    // 버퍼가 비어있는 경우
    if (count == 0)
    {return false;}
    data = buffer[read_idx];
    read_idx++;
    if (read_idx >= RING_BUFFER_SIZE)
    {read_idx = 0;}
    count--;
    return true;
}

size_t RingBuffer::read(uint8_t* data, size_t len)
{
    size_t read_count = 0;
    while (read_count < len)
    {
        if (!read(data[read_count]))
        {break;}
        read_count++;
    }
    return read_count;
}


bool RingBuffer::peek(size_t offset, uint8_t& data) const
{
    if (offset >= count)
    {return false;}
    size_t index = read_idx + offset;
    if (index >= RING_BUFFER_SIZE)
    {index -= RING_BUFFER_SIZE;}
    data = buffer[index];
    return true;
}

size_t RingBuffer::available() const
{return count;}

size_t RingBuffer::free_space() const
{return RING_BUFFER_SIZE - count;}

void RingBuffer::clear()
{
    write_idx = 0;
    read_idx = 0;
    count = 0;
}
