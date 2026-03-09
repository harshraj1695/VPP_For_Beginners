// #include "icmp.h"
// #include <vnet/plugin/plugin.h>
// #include <vpp/app/version.h>

// /* ── checksum ──────────────────────────────────────────────── */
// static u16
// icmp_csum (void *data, u32 len)
// {
//     u32 sum = 0;
//     u16 *p  = data;
//     while (len > 1) { sum += *p++; len -= 2; }
//     if (len)         { sum += *(u8 *)p; }
//     while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);
//     return ~(u16)sum;
// }

// /* ── trace ─────────────────────────────────────────────────── */
// typedef struct {
//     u8  action;   /* 0=dropped, 1=replied */
//     u8  icmp_type;
//     u16 seq;
//     ip4_address_t src;
//     ip4_address_t dst;
// } icmp_trace_t;

// static u8 *
// format_icmp_trace (u8 *s, va_list *args)
// {
//     CLIB_UNUSED (vlib_main_t * vm)   = va_arg (*args, vlib_main_t *);
//     CLIB_UNUSED (vlib_node_t * node) = va_arg (*args, vlib_node_t *);
//     icmp_trace_t *t = va_arg (*args, icmp_trace_t *);

//     s = format (s, "icmp-input: %U -> %U type=%d seq=%d action=%s",
//                 format_ip4_address, &t->src,
//                 format_ip4_address, &t->dst,
//                 t->icmp_type, t->seq,
//                 t->action ? "REPLIED" : "DROPPED");
//     return s;
// }

// /* ── node function ─────────────────────────────────────────── */
// static uword
// icmp_node_fn (vlib_main_t *vm,
//               vlib_node_runtime_t *node,
//               vlib_frame_t *frame)
// {
//     u32 *from        = vlib_frame_vector_args (frame);
//     u32  n           = frame->n_vectors;
//     ip4_address_t me = MY_IP4_ADDR;

//     while (n--)
//     {
//         vlib_buffer_t *b    = vlib_get_buffer (vm, from[n]);
//         ip4_header_t  *ip   = vlib_buffer_get_current (b);
//         icmp_echo_t   *icmp = (icmp_echo_t *)(ip + 1);
//         u32 next = NEXT_DROP;

//         /* drop everything that is not ICMP Echo Request to 10.10.1.1 */
//         if (ip->protocol           != IP_PROTO_ICMP)     goto trace;
//         if (ip->dst_address.as_u32 != me.as_u32)         goto trace;
//         if (icmp->type             != ICMP_ECHO_REQUEST)  goto trace;

//         /* build reply in-place: swap IPs, flip type, recompute checksums */
//         {
//             ip4_address_t tmp = ip->src_address;
//             ip->src_address   = ip->dst_address;
//             ip->dst_address   = tmp;
//         }
//         ip->checksum = 0;
//         ip->checksum = ip4_header_checksum (ip);

//         icmp->type     = ICMP_ECHO_REPLY;
//         icmp->checksum = 0;
//         icmp->checksum = icmp_csum (icmp,
//                              clib_net_to_host_u16 (ip->length)
//                              - ip4_header_bytes (ip));

//         /* rewind buffer back to ethernet header (14 bytes before IP) */
//         vlib_buffer_advance (b, -(i32) sizeof (ethernet_header_t));
//         ethernet_header_t *eth = vlib_buffer_get_current (b);
//         u8 tmp_mac[6];
//         clib_memcpy (tmp_mac,          eth->dst_address, 6);
//         clib_memcpy (eth->dst_address, eth->src_address, 6);
//         clib_memcpy (eth->src_address, tmp_mac,          6);

//         vnet_buffer (b)->sw_if_index[VLIB_TX] =
//             vnet_buffer (b)->sw_if_index[VLIB_RX];
//         next = NEXT_TX;

//     trace:
//         if (PREDICT_FALSE (b->flags & VLIB_BUFFER_IS_TRACED))
//         {
//             icmp_trace_t *t = vlib_add_trace (vm, node, b, sizeof (*t));
//             t->action    = (next == NEXT_TX);
//             t->icmp_type = icmp->type;
//             t->seq       = clib_net_to_host_u16 (icmp->seq);
//             t->src       = ip->src_address;
//             t->dst       = ip->dst_address;
//         }

//         vlib_set_next_frame_buffer (vm, node, next, from[n]);
//     }

//     return frame->n_vectors;
// }

// /* ── registration ──────────────────────────────────────────── */
// VLIB_REGISTER_NODE (icmp_node) = {
//     .function      = icmp_node_fn,
//     .name          = "icmp-input",
//     .vector_size   = sizeof (u32),
//     .format_trace  = format_icmp_trace,
//     .type          = VLIB_NODE_TYPE_INTERNAL,
//     .n_next_nodes  = NEXT_N,
//     .next_nodes    = {
//         [NEXT_TX]   = "interface-output",
//         [NEXT_DROP] = "error-drop",
//     },
// };

// /*
//  * ip4-local arc: packets TO a local IP go through ip4-input -> ip4-local.
//  * We insert here so we intercept ICMP before VPP's built-in ip4-icmp-input.
//  */
// VNET_FEATURE_INIT (icmp_feat, static) = {
//     .arc_name    = "ip4-local",
//     .node_name   = "icmp-input",
//     .runs_before = VNET_FEATURES ("ip4-icmp-input"),
// };

// VLIB_PLUGIN_REGISTER () = {
//     .version     = VPP_BUILD_VER,
//     .description = "ICMP echo handler for 10.10.1.1 — drops everything else",
// };


#include "icmp.h"
#include <vnet/plugin/plugin.h>
#include <vpp/app/version.h>

#define CUSTOM_MSG      "harsh has done it... "
#define CUSTOM_MSG_LEN  (sizeof (CUSTOM_MSG) - 1)   /* exclude null terminator */

/* ── checksum ──────────────────────────────────────────────── */
static u16
icmp_csum (void *data, u32 len)
{
    u32 sum = 0;
    u16 *p  = data;
    while (len > 1) { sum += *p++; len -= 2; }
    if (len)         { sum += *(u8 *)p; }
    while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);
    return ~(u16)sum;
}

/* ── trace ─────────────────────────────────────────────────── */
typedef struct {
    u8  action;   /* 0=dropped, 1=replied */
    u8  icmp_type;
    u16 seq;
    ip4_address_t src;
    ip4_address_t dst;
} icmp_trace_t;

static u8 *
format_icmp_trace (u8 *s, va_list *args)
{
    CLIB_UNUSED (vlib_main_t * vm)   = va_arg (*args, vlib_main_t *);
    CLIB_UNUSED (vlib_node_t * node) = va_arg (*args, vlib_node_t *);
    icmp_trace_t *t = va_arg (*args, icmp_trace_t *);

    s = format (s, "icmp-input: %U -> %U type=%d seq=%d action=%s",
                format_ip4_address, &t->src,
                format_ip4_address, &t->dst,
                t->icmp_type, t->seq,
                t->action ? "REPLIED [" CUSTOM_MSG "]" : "DROPPED");
    return s;
}

/* ── node function ─────────────────────────────────────────── */
static uword
icmp_node_fn (vlib_main_t *vm,
              vlib_node_runtime_t *node,
              vlib_frame_t *frame)
{
    u32 *from        = vlib_frame_vector_args (frame);
    u32  n           = frame->n_vectors;
    ip4_address_t me = MY_IP4_ADDR;

    while (n--)
    {
        vlib_buffer_t *b    = vlib_get_buffer (vm, from[n]);
        ip4_header_t  *ip   = vlib_buffer_get_current (b);
        icmp_echo_t   *icmp = (icmp_echo_t *)(ip + 1);
        u32 next = NEXT_DROP;

        /* drop everything that is not ICMP Echo Request to 10.10.1.1 */
        if (ip->protocol           != IP_PROTO_ICMP)      goto trace;
        if (ip->dst_address.as_u32 != me.as_u32)          goto trace;
        if (icmp->type             != ICMP_ECHO_REQUEST)   goto trace;

        /* stamp custom payload right after the 8-byte ICMP header */
        {
            u8   *payload     = (u8 *)(icmp + 1);
            u32   payload_len = clib_net_to_host_u16 (ip->length)
                                - ip4_header_bytes (ip)
                                - sizeof (icmp_echo_t);
            u32   copy_len    = payload_len < CUSTOM_MSG_LEN
                                ? payload_len : CUSTOM_MSG_LEN;
            clib_memcpy (payload, CUSTOM_MSG, copy_len);
        }

        /* build reply: swap IPs, flip type 8→0, recompute checksums */
        {
            ip4_address_t tmp = ip->src_address;
            ip->src_address   = ip->dst_address;
            ip->dst_address   = tmp;
        }
        ip->checksum = 0;
        ip->checksum = ip4_header_checksum (ip);

        icmp->type     = ICMP_ECHO_REPLY;
        icmp->checksum = 0;
        icmp->checksum = icmp_csum (icmp,
                             clib_net_to_host_u16 (ip->length)
                             - ip4_header_bytes (ip));

        /* rewind to ethernet header, swap MACs */
        vlib_buffer_advance (b, -(i32) sizeof (ethernet_header_t));
        {
            ethernet_header_t *eth = vlib_buffer_get_current (b);
            u8 tmp_mac[6];
            clib_memcpy (tmp_mac,          eth->dst_address, 6);
            clib_memcpy (eth->dst_address, eth->src_address, 6);
            clib_memcpy (eth->src_address, tmp_mac,          6);
        }

        vnet_buffer (b)->sw_if_index[VLIB_TX] =
            vnet_buffer (b)->sw_if_index[VLIB_RX];
        next = NEXT_TX;

    trace:
        if (PREDICT_FALSE (b->flags & VLIB_BUFFER_IS_TRACED))
        {
            icmp_trace_t *t = vlib_add_trace (vm, node, b, sizeof (*t));
            t->action    = (next == NEXT_TX);
            t->icmp_type = icmp->type;
            t->seq       = clib_net_to_host_u16 (icmp->seq);
            t->src       = ip->src_address;
            t->dst       = ip->dst_address;
        }

        vlib_set_next_frame_buffer (vm, node, next, from[n]);
    }

    return frame->n_vectors;
}

/* ── registration ──────────────────────────────────────────── */
VLIB_REGISTER_NODE (icmp_node) = {
    .function      = icmp_node_fn,
    .name          = "icmp-input",
    .vector_size   = sizeof (u32),
    .format_trace  = format_icmp_trace,
    .type          = VLIB_NODE_TYPE_INTERNAL,
    .n_next_nodes  = NEXT_N,
    .next_nodes    = {
        [NEXT_TX]   = "interface-output",
        [NEXT_DROP] = "error-drop",
    },
};

VNET_FEATURE_INIT (icmp_feat, static) = {
    .arc_name    = "ip4-local",
    .node_name   = "icmp-input",
    .runs_before = VNET_FEATURES ("ip4-icmp-input"),
};

VLIB_PLUGIN_REGISTER () = {
    .version     = VPP_BUILD_VER,
    .description = "ICMP echo handler for 10.10.1.1 — drops everything else",
};