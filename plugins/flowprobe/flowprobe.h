#ifndef __included_flowprobe_h__
#define __included_flowprobe_h__

#include <vlib/vlib.h>
#include <vnet/vnet.h>
#include <vnet/ip/ip.h>

#define FLOW_RING_SIZE 65536

typedef struct {
    u32 src;
    u32 dst;
    u32 bytes;
    f64 timestamp;
} flow_record_t;

typedef struct {
    flow_record_t *records;
    u32 head;
    u32 tail;
} flow_ring_t;

typedef struct {
    flow_ring_t *rings;   // per worker
    u32 n_workers;
    u8 enabled;
    FILE *fp;
    u64 dropped;
} flowprobe_main_t;

extern flowprobe_main_t flowprobe_main;

#endif