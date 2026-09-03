#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

#define PACKET_MAX_LEN 100

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

#endif