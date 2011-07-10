#ifndef __NET_H__
#define __NET_H__

#define OGM_VERSION 1

typedef enum
{
  UNICAST, BROADCAST
} llc_packet_type_t;

typedef struct
{
  llc_packet_type_t type :4;
  uint16_t len :12;
  uint16_t crc;
  uint8_t *data;
} llc_packet_t;

typedef uint16_t addr_t;

#define OGM_FLAG_IS_DIRECT 0
#define OGM_FLAG_UNIDIRECTIONAL 1

typedef struct
{
  uint8_t version :4;
  uint8_t flags :4;
  uint8_t ttl;
  uint16_t seqno;
  addr_t originator_addr;
  addr_t sender_addr;
} ogm_packet_t;

#endif
