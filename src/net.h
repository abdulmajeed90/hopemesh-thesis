#ifndef __NET_H__
#define __NET_H__

typedef uint16_t addr_t;

typedef struct {
  uint8_t flags;
  uint8_t ttl;
  uint16_t seqno;
  addr_t originator_addr;
  addr_t sender_addr;
} ogm_packet_t;

#endif
