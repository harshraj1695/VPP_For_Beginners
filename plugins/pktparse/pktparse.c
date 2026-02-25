#include <vlib/unix/plugin.h>
#include <vlib/vlib.h>
#include <vnet/vnet.h>

#include <vnet/ethernet/ethernet.h>
#include <vnet/ip/ip4_packet.h>
#include <vnet/ip/format.h>
#include <vnet/tcp/tcp_packet.h>
#include <vnet/udp/udp_packet.h>

#include "pktparse.h"

/* =====================================================
   Global state (per interface enable bitmap)
   ===================================================== */

typedef struct {
    u8 *enabled_by_sw_if_index;
} pktparse_main_t;

pktparse_main_t pktparse_main;


/* =====================================================
   Node function
   ===================================================== */

static uword
pktparse_node_fn (vlib_main_t *vm,
                  vlib_node_runtime_t *node,
                  vlib_frame_t *frame)
{
    pktparse_main_t *pm = &pktparse_main;

    u32 *from = vlib_frame_vector_args(frame);
    u32 n_left_from = frame->n_vectors;

    while (n_left_from > 0)
    {
        u32 bi0 = from[0];
        from++;
        n_left_from--;

        vlib_buffer_t *b0 = vlib_get_buffer(vm, bi0);

        /* get incoming interface */
        u32 sw_if_index = vnet_buffer(b0)->sw_if_index[VLIB_RX];

        /* if not enabled → just forward */
        if (sw_if_index >= vec_len(pm->enabled_by_sw_if_index) ||
            pm->enabled_by_sw_if_index[sw_if_index] == 0)
        {
            vlib_buffer_enqueue_to_single_next(vm, node, &bi0, 0, 1);
            continue;
        }

        /* ================= PARSE PACKET ================= */

        void *data = vlib_buffer_get_current(b0);
        ethernet_header_t *eth = data;

        if (clib_net_to_host_u16(eth->type) == ETHERNET_TYPE_IP4)
        {
            ip4_header_t *ip4 = (ip4_header_t *)(eth + 1);

            ip4_address_t src = ip4->src_address;
            ip4_address_t dst = ip4->dst_address;

            u8 proto = ip4->protocol;

            vlib_cli_output(vm,
                "pktparse: IPv4 src=%U dst=%U proto=%u",
                format_ip4_address, &src,
                format_ip4_address, &dst,
                proto);

            if (proto == IP_PROTOCOL_TCP)
            {
                tcp_header_t *tcp =
                    (tcp_header_t *)((u8 *)ip4 + ip4_header_bytes(ip4));

                vlib_cli_output(vm,
                    "pktparse: TCP sport=%u dport=%u",
                    clib_net_to_host_u16(tcp->src_port),
                    clib_net_to_host_u16(tcp->dst_port));
            }
            else if (proto == IP_PROTOCOL_UDP)
            {
                udp_header_t *udp =
                    (udp_header_t *)((u8 *)ip4 + ip4_header_bytes(ip4));

                vlib_cli_output(vm,
                    "pktparse: UDP sport=%u dport=%u",
                    clib_net_to_host_u16(udp->src_port),
                    clib_net_to_host_u16(udp->dst_port));
            }
        }

        /* always forward */
        vlib_buffer_enqueue_to_single_next(vm, node, &bi0, 0, 1);
    }

    return frame->n_vectors;
}


/* =====================================================
   Node registration
   ===================================================== */

VLIB_REGISTER_NODE (pktparse_node) = {
    .function = pktparse_node_fn,
    .name = "pktparse-node",
    .vector_size = sizeof(u32),
    .type = VLIB_NODE_TYPE_INTERNAL,

    .n_next_nodes = 1,
    .next_nodes = {
        [0] = "ethernet-input",
    },
};


/* =====================================================
   Feature hook (always attached, but gated by flag)
   ===================================================== */

VNET_FEATURE_INIT (pktparse_feature, static) = {
    .arc_name = "device-input",
    .node_name = "pktparse-node",
    .runs_before = VNET_FEATURES ("ethernet-input"),
};


/* =====================================================
   CLI command
   ===================================================== */

static clib_error_t *
pktparse_enable_disable (vlib_main_t *vm,
                         unformat_input_t *input,
                         vlib_cli_command_t *cmd)
{
    pktparse_main_t *pm = &pktparse_main;

    u32 sw_if_index = ~0;
    u8 enable = 1;

    while (unformat_check_input(input) != UNFORMAT_END_OF_INPUT)
    {
        if (unformat(input, "disable"))
            enable = 0;
        else if (unformat(input, "%U",
                          unformat_vnet_sw_interface,
                          vnet_get_main(),
                          &sw_if_index))
            ;
        else
            return clib_error_return(0, "unknown input");
    }

    if (sw_if_index == ~0)
        return clib_error_return(0, "interface required");

    vec_validate(pm->enabled_by_sw_if_index, sw_if_index);
    pm->enabled_by_sw_if_index[sw_if_index] = enable;

    vlib_cli_output(vm,
        "pktparse %s on interface %u",
        enable ? "enabled" : "disabled",
        sw_if_index);

    return 0;
}

VLIB_CLI_COMMAND (pktparse_cmd, static) = {
    .path = "pktparse",
    .short_help = "pktparse <interface> [disable]",
    .function = pktparse_enable_disable,
};


/* =====================================================
   Plugin registration
   ===================================================== */

VLIB_PLUGIN_REGISTER () = {
    .version = "1.0",
    .description = "Packet Parser Plugin with CLI Enable",
};
