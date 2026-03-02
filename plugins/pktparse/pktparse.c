#include <vlib/vlib.h>
#include <vnet/vnet.h>

#include <vnet/ethernet/ethernet.h>
#include <vnet/ip/ip4_packet.h>
#include <vnet/tcp/tcp_packet.h>
#include <vnet/udp/udp_packet.h>
#include <vlib/unix/plugin.h>
#include <vnet/ip/format.h>

// global enable flag
static u8 pktparse_enabled = 0;

// node function
static uword
pktparse_node_fn (vlib_main_t *vm,
                  vlib_node_runtime_t *node,
                  vlib_frame_t *frame)
{
    u32 *from = vlib_frame_vector_args(frame);
    u32 n_left = frame->n_vectors;

    while (n_left > 0)
    {
        u32 bi0 = from[0];
        from++;
        n_left--;

        // if disabled just forward
        if (!pktparse_enabled)
        {
            vlib_buffer_enqueue_to_single_next(vm, node, &bi0, 0, 1);
            continue;
        }

        vlib_buffer_t *b0 = vlib_get_buffer(vm, bi0);
        void *data = vlib_buffer_get_current(b0);
        ethernet_header_t *eth = data;

        // print MAC addresses
        vlib_cli_output(vm,
            "MAC src=%U dst=%U",
            format_ethernet_address, eth->src_address,
            format_ethernet_address, eth->dst_address);

        if (clib_net_to_host_u16(eth->type) == ETHERNET_TYPE_IP4)
        {
            ip4_header_t *ip4 = (ip4_header_t *)(eth + 1);

            vlib_cli_output(vm,
                "IPv4 src=%U dst=%U proto=%u",
                format_ip4_address, &ip4->src_address,
                format_ip4_address, &ip4->dst_address,
                ip4->protocol);

            if (ip4->protocol == IP_PROTOCOL_TCP)
            {
                tcp_header_t *tcp =
                    (tcp_header_t *)((u8 *)ip4 + ip4_header_bytes(ip4));

                vlib_cli_output(vm,
                    "TCP sport=%u dport=%u",
                    clib_net_to_host_u16(tcp->src_port),
                    clib_net_to_host_u16(tcp->dst_port));
            }
            else if (ip4->protocol == IP_PROTOCOL_UDP)
            {
                udp_header_t *udp =
                    (udp_header_t *)((u8 *)ip4 + ip4_header_bytes(ip4));

                vlib_cli_output(vm,
                    "UDP sport=%u dport=%u",
                    clib_net_to_host_u16(udp->src_port),
                    clib_net_to_host_u16(udp->dst_port));
            }
        }

        vlib_buffer_enqueue_to_single_next(vm, node, &bi0, 0, 1);
    }

    return frame->n_vectors;
}

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

VNET_FEATURE_INIT (pktparse_feature, static) = {
    .arc_name = "device-input",
    .node_name = "pktparse-node",
    .runs_before = VNET_FEATURES ("ethernet-input"),
};

// CLI enable disable
static clib_error_t *
pktparse_enable_disable (vlib_main_t *vm,
                         unformat_input_t *input,
                         vlib_cli_command_t *cmd)
{
    if (unformat(input, "enable"))
        pktparse_enabled = 1;
    else if (unformat(input, "disable"))
        pktparse_enabled = 0;
    else
        return clib_error_return(0, "use: pktparse enable|disable");

    vlib_cli_output(vm,
        "pktparse %s",
        pktparse_enabled ? "enabled" : "disabled");

    return 0;
}

VLIB_CLI_COMMAND (pktparse_cmd, static) = {
    .path = "pktparse",
    .short_help = "pktparse enable|disable",
    .function = pktparse_enable_disable,
};

VLIB_PLUGIN_REGISTER () = {
    .version = "1.0",
    .description = "Packet Parser Plugin with MAC",
};