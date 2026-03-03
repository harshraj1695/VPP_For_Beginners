#include <vlib/vlib.h>
#include <vnet/vnet.h>
#include <vnet/ethernet/ethernet.h>
#include <vnet/arp/arp.h>
#include <vlib/unix/plugin.h>

// our IP = 10.0.0.2 (network byte order)
static u32 my_ip = 0x0a000002;

// our custom MAC
static u8 my_mac[6] = {0x02, 0xaa, 0xbb, 0xcc, 0xdd, 0xee};

typedef struct {
  u32 sw_if_index;
} my_trace_t;

static u8 *
format_my_trace (u8 *s, va_list *args)
{
  my_trace_t *t = va_arg (*args, my_trace_t *);
  return format (s, "myarp: sw_if_index %u", t->sw_if_index);
}

static uword
myarp_node_fn (vlib_main_t *vm,
               vlib_node_runtime_t *node,
               vlib_frame_t *frame)
{
  u32 *from = vlib_frame_vector_args(frame);
  u32 n_left = frame->n_vectors;

  while (n_left > 0)
  {
    u32 bi = from[0];
    vlib_buffer_t *b = vlib_get_buffer(vm, bi);

    ethernet_arp_header_t *arp =
      vlib_buffer_get_current(b);

    // Default: continue to arp-reply
    u16 next = 0;

    // Only process ARP request
    if (arp->opcode == clib_host_to_net_u16(1))
    {
      u32 target_ip =
        arp->ip4_over_ethernet[1].ip4.as_u32;

      if (target_ip == my_ip)
      {
        // Ethernet header is before ARP header
        ethernet_header_t *eth =
          (ethernet_header_t *)(arp - 1);

        // Save sender MAC/IP
        u8 sender_mac[6];
        clib_memcpy(sender_mac,
                    arp->ip4_over_ethernet[0].mac.bytes,
                    6);

        u32 sender_ip =
          arp->ip4_over_ethernet[0].ip4.as_u32;

        // Swap Ethernet header
        clib_memcpy(eth->dst_address, sender_mac, 6);
        clib_memcpy(eth->src_address, my_mac, 6);

        // Convert to ARP reply
        arp->opcode = clib_host_to_net_u16(2);

        // Sender = us
        clib_memcpy(
          arp->ip4_over_ethernet[0].mac.bytes,
          my_mac, 6);
        arp->ip4_over_ethernet[0].ip4.as_u32 = my_ip;

        // Target = original sender
        clib_memcpy(
          arp->ip4_over_ethernet[1].mac.bytes,
          sender_mac, 6);
        arp->ip4_over_ethernet[1].ip4.as_u32 = sender_ip;
      }
    }

    // Trace support
    if (b->flags & VLIB_BUFFER_IS_TRACED)
    {
      my_trace_t *t =
        vlib_add_trace(vm, node, b, sizeof(*t));
      t->sw_if_index =
        vnet_buffer(b)->sw_if_index[VLIB_RX];
    }

    vlib_set_next_frame_buffer(vm, node, next, bi);

    from++;
    n_left--;
  }

  return frame->n_vectors;
}

VLIB_REGISTER_NODE (myarp_node) = {
  .name = "myarp-node",
  .function = myarp_node_fn,
  .vector_size = sizeof(u32),
  .format_trace = format_my_trace,
  .type = VLIB_NODE_TYPE_INTERNAL,
  .flags = VLIB_NODE_FLAG_TRACE_SUPPORTED,

  .n_next_nodes = 1,
  .next_nodes = {
    [0] = "arp-reply",
  },
};

// Attach to ARP feature arc correctly
VNET_FEATURE_INIT (myarp_feature, static) = {
  .arc_name = "arp",
  .node_name = "myarp-node",
  .runs_before = VNET_FEATURES ("arp-reply"),
};

VLIB_PLUGIN_REGISTER () = {
  .version = "1.0",
  .description = "Custom ARP responder (stable/2506)",
};