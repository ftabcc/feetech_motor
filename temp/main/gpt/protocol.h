#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

#define PACKET_MAX_LEN 100
#define RING_BUFFER_SIZE 256

typedef struct
{
    uint8_t buffer[PACKET_MAX_LEN + 3];
    size_t len;// packet_total_LEN = HEAD(2) + LEN(1) + INST(1) + DATA(N) + CHECKSUM(1) = N+5 (N>=0)
    size_t idx;

} packet_t;


class protocol
{
public:
    static bool packet_parser(packet_t *packet,uint8_t byte);
};



class RingBuffer
{
private:
    uint8_t buffer[RING_BUFFER_SIZE];

    size_t write_idx;
    size_t read_idx;
    size_t count;

public:
    RingBuffer();

    bool write(uint8_t data);
    size_t write(const uint8_t* data, size_t len);

    bool read(uint8_t& data);
    size_t read(uint8_t* data, size_t len);

    bool peek(size_t offset, uint8_t& data) const;

    size_t available() const;
    size_t free_space() const;

    void clear();
};

#endif

#endif