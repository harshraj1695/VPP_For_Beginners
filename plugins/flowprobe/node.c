#include <vlib/vlib.h>
#include <vnet/vnet.h>
#include <vnet/ip/ip.h>
#include "flowprobe.h"

static_always_inline void
flowprobe_enqueue(flow_ring_t *ring,
                  flow_record_t *r)
{
    u32 next = (ring->head + 1) % FLOW_RING_SIZE;

    if (next != ring->tail)
    {
        ring->records[ring->head] = *r;
        ring->head = next;
    }
    else
    {
        flowprobe_main.dropped++;
    }
}

VLIB_NODE_FN(flowprobe_node)
(vlib_main_t *vm,
 vlib_node_runtime_t *node,
 vlib_frame_t *frame)
{
    flowprobe_main_t *fm = &flowprobe_main;

    if (!fm->enabled)
        return frame->n_vectors;

    u32 *from = vlib_frame_vector_args(frame);
    u32 n_left = frame->n_vectors;

    u32 thread_id = vlib_get_thread_index();
    flow_ring_t *ring = &fm->rings[thread_id];

    while (n_left > 0)
    {
        u32 bi0 = from[0];
        from += 1;
        n_left -= 1;

        vlib_buffer_t *b0 = vlib_get_buffer(vm, bi0);
        ip4_header_t *ip =
            vlib_buffer_get_current(b0);

        flow_record_t r;

        r.src = ip->src_address.as_u32;
        r.dst = ip->dst_address.as_u32;
        r.bytes = vlib_buffer_length_in_chain(vm, b0);
        r.timestamp = vlib_time_now(vm);

        flowprobe_enqueue(ring, &r);
    }

    return frame->n_vectors;
}

VLIB_REGISTER_NODE(flowprobe_node) = {
    .name = "flowprobe-node",
    .vector_size = sizeof(u32),
    .type = VLIB_NODE_TYPE_INTERNAL,
};


VNET_FEATURE_INIT(flowprobe_feature, static) = {
    .arc_name = "ip4-unicast",
    .node_name = "flowprobe-node",
    .runs_before = VNET_FEATURES("ip4-lookup"),
};