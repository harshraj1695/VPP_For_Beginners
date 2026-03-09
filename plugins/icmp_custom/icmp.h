#ifndef __icmp_h__
#define __icmp_h__

#include <vlib/vlib.h>
#include <vnet/vnet.h>
#include <vnet/ip/ip4_packet.h>
#include <vnet/ip/format.h>
#include <vnet/ethernet/ethernet.h>

/* Our node's IP — only reply to packets destined here */
#define MY_IP4_ADDR  { .as_u8 = { 10, 10, 1, 1 } }

#define IP_PROTO_ICMP       1
#define ICMP_ECHO_REQUEST   8
#define ICMP_ECHO_REPLY     0

typedef CLIB_PACKED (struct {
    u8  type;
    u8  code;
    u16 checksum;
    u16 id;
    u16 seq;
}) icmp_echo_t;

typedef enum {
    NEXT_TX,
    NEXT_DROP,
    NEXT_N,
} next_t;

extern vlib_node_registration_t icmp_node;

#endif