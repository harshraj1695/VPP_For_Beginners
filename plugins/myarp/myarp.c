#include <vlib/vlib.h>
#include <vnet/vnet.h>
#include <vnet/ethernet/ethernet.h>
#include <vnet/arp/arp.h>
#include <vlib/unix/plugin.h>

/*
 * REAL values taken from your setup:
 *   VPP tap0 MAC  : 02:fe:3c:6d:95:1f   (from: ip link show vpp-tap0)
 *   VPP IP        : 10.10.1.1
 *   Linux vpp-tap0: 10.10.1.2/24
 *
 * my_ip is stored in NETWORK byte order so it matches
 * arp->ip4_over_ethernet[x].ip4.as_u32 directly off the wire.
 */

static u8 my_mac[6] = {0x02, 0xfe, 0x3c, 0x6d, 0x95, 0x1f};
static u32 my_ip;   /* set in init function below */

typedef enum {
  MYARP_NEXT_DROP = 0,
  MYARP_NEXT_IFACE_OUTPUT = 1,
  MYARP_N_NEXT,
} myarp_next_t;

static uword
myarp_node_fn (vlib_main_t *vm,
               vlib_node_runtime_t *node,
               vlib_frame_t *frame)
{
  u32 *from = vlib_frame_vector_args (frame);
  u32 n_left = frame->n_vectors;

  while (n_left > 0)
    {
      u32 bi = from[0];
      vlib_buffer_t *b = vlib_get_buffer (vm, bi);

      /*
       * In device-input, vlib_buffer_get_current() already points
       * at the Ethernet header. Do NOT call vlib_buffer_advance here.
       */
      ethernet_header_t *eth = vlib_buffer_get_current (b);

      myarp_next_t next = MYARP_NEXT_DROP;

      /* is this an ARP packet? */
      if (clib_net_to_host_u16 (eth->type) == ETHERNET_TYPE_ARP)
        {
          ethernet_arp_header_t *arp =
            (ethernet_arp_header_t *) (eth + 1);

          /* is this an ARP REQUEST (opcode 1)? */
          if (clib_net_to_host_u16 (arp->opcode) == 1)
            {
              u32 sender_ip = arp->ip4_over_ethernet[0].ip4.as_u32;
              u32 target_ip = arp->ip4_over_ethernet[1].ip4.as_u32;

              /*
               * target_ip and my_ip are both in network byte order
               * so this comparison is correct.
               * Also guard sender_ip != 0 to skip gratuitous ARPs.
               */
              if (target_ip == my_ip && sender_ip != 0)
                {
                  u8 sender_mac[6];

                  /* save original sender MAC before overwriting */
                  clib_memcpy (sender_mac,
                               arp->ip4_over_ethernet[0].mac.bytes, 6);

                  /* --- patch Ethernet header --- */
                  clib_memcpy (eth->dst_address, sender_mac, 6);
                  clib_memcpy (eth->src_address, my_mac, 6);

                  /* --- patch ARP body: request -> reply --- */
                  arp->opcode = clib_host_to_net_u16 (2);

                  /* sender = us */
                  clib_memcpy (arp->ip4_over_ethernet[0].mac.bytes,
                               my_mac, 6);
                  arp->ip4_over_ethernet[0].ip4.as_u32 = my_ip;

                  /* target = original sender */
                  clib_memcpy (arp->ip4_over_ethernet[1].mac.bytes,
                               sender_mac, 6);
                  arp->ip4_over_ethernet[1].ip4.as_u32 = sender_ip;

                  /* send reply back out the same interface */
                  vnet_buffer (b)->sw_if_index[VLIB_TX] =
                    vnet_buffer (b)->sw_if_index[VLIB_RX];

                  next = MYARP_NEXT_IFACE_OUTPUT;
                }
            }
        }

      vlib_set_next_frame_buffer (vm, node, next, bi);

      from++;
      n_left--;
    }

  return frame->n_vectors;
}

/* ----------------------------------------------------------------
 * Init: store my_ip in network byte order once at startup
 * ---------------------------------------------------------------- */
static clib_error_t *
myarp_init (vlib_main_t *vm)
{
  /* 10.10.1.1 in host order -> stored as network order */
  my_ip = clib_host_to_net_u32 (0x0a0a0101);
  return 0;
}

VLIB_INIT_FUNCTION (myarp_init);

/* ----------------------------------------------------------------
 * Node registration
 * ---------------------------------------------------------------- */
VLIB_REGISTER_NODE (myarp_node) = {
  .name         = "myarp-node",
  .function     = myarp_node_fn,
  .vector_size  = sizeof (u32),
  .n_next_nodes = MYARP_N_NEXT,
  .next_nodes   = {
    [MYARP_NEXT_DROP]         = "error-drop",
    [MYARP_NEXT_IFACE_OUTPUT] = "interface-output",
  },
};

/* ----------------------------------------------------------------
 * Feature registration
 * arc  : device-input  (raw packets, before ANY VPP logic)
 * runs before ethernet-input so we intercept first
 * ---------------------------------------------------------------- */
VNET_FEATURE_INIT (myarp_feature, static) = {
  .arc_name    = "device-input",
  .node_name   = "myarp-node",
  .runs_before = VNET_FEATURES ("ethernet-input"),
};

VLIB_PLUGIN_REGISTER () = {
  .version     = "1.0",
  .description = "Simple ARP responder — no built-in VPP ARP logic",
};