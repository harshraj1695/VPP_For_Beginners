#include <vlib/vlib.h>
#include <vnet/vnet.h>
#include <vnet/ethernet/ethernet.h>
#include <vnet/arp/arp.h>
#include <vlib/unix/plugin.h>

/*
 * REAL values taken from my setup:
 *   VPP tap0 MAC  : 02:fe:3c:6d:95:1f   (from: ip link show vpp-tap0)
 *   VPP IP        : 10.10.1.1
 *   Linux vpp-tap0: 10.10.1.2/24
 *
 * arp->ip4_over_ethernet[x].ip4.as_u32 directly off the wire.
 */

static u8  my_mac[6] = {0x02, 0xfe, 0x3c, 0x6d, 0x95, 0x1f};
static u32 my_ip;   /* set in network byte order by myarp_init() */

 // Per-packet trace record
typedef struct {
  u32 sw_if_index;
  u32 next_index;
  u8  is_arp_reply;   /* 1 = we built a reply, 0 = dropped */
  u8  sender_mac[6];
  u32 sender_ip;
  u32 target_ip;
} myarp_trace_t;

static u8 *
format_myarp_trace (u8 *s, va_list *args)
{
  CLIB_UNUSED (vlib_main_t  *vm)   = va_arg (*args, vlib_main_t *);
  CLIB_UNUSED (vlib_node_t  *node) = va_arg (*args, vlib_node_t *);
  myarp_trace_t *t = va_arg (*args, myarp_trace_t *);

  if (t->is_arp_reply)
    s = format (s,
                "myarp: REPLY sent  sw_if_index %d  "
                "sender %U/%U  target_ip %U",
                t->sw_if_index,
                format_mac_address, t->sender_mac,
                format_ip4_address, &t->sender_ip,
                format_ip4_address, &t->target_ip);
  else
    s = format (s,
                "myarp: DROP        sw_if_index %d  next %d",
                t->sw_if_index, t->next_index);
  return s;
}

 // Next-node indices
typedef enum {
  MYARP_NEXT_DROP = 0,
  MYARP_NEXT_IFACE_OUTPUT = 1,
  MYARP_N_NEXT,
} myarp_next_t;

 // Node function
static uword
myarp_node_fn (vlib_main_t        *vm,
               vlib_node_runtime_t *node,
               vlib_frame_t        *frame)
{
  u32 *from  = vlib_frame_vector_args (frame);
  u32  n_left = frame->n_vectors;

  while (n_left > 0)
    {
      u32            bi = from[0];
      vlib_buffer_t *b  = vlib_get_buffer (vm, bi);

      /*
       * In device-input the current pointer already sits at the
       * Ethernet header — do NOT call vlib_buffer_advance here.
       */
      ethernet_header_t *eth = vlib_buffer_get_current (b);

      myarp_next_t next          = MYARP_NEXT_DROP;
      u8           is_arp_reply  = 0;
      u8           sender_mac[6] = {0};
      u32          sender_ip     = 0;
      u32          target_ip     = 0;

      /* ---- is this ARP? ---- */
      if (clib_net_to_host_u16 (eth->type) == ETHERNET_TYPE_ARP)
        {
          ethernet_arp_header_t *arp =
            (ethernet_arp_header_t *) (eth + 1);

          /* ---- is this an ARP REQUEST (opcode 1)? ---- */
          if (clib_net_to_host_u16 (arp->opcode) == 1)
            {
              sender_ip = arp->ip4_over_ethernet[0].ip4.as_u32;
              target_ip = arp->ip4_over_ethernet[1].ip4.as_u32;

              /*
               * Both target_ip and my_ip are in network byte order
               * so the comparison is correct.
               * Guard sender_ip != 0 to skip gratuitous ARPs.
               */
              if (target_ip == my_ip && sender_ip != 0)
                {
                  /* save sender MAC before overwriting */
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

                  next         = MYARP_NEXT_IFACE_OUTPUT;
                  is_arp_reply = 1;
                }
            }
        }

      /* ---- trace ---- */
      if (PREDICT_FALSE (b->flags & VLIB_BUFFER_IS_TRACED))
        {
          myarp_trace_t *t = vlib_add_trace (vm, node, b, sizeof (*t));
          t->sw_if_index  = vnet_buffer (b)->sw_if_index[VLIB_RX];
          t->next_index   = next;
          t->is_arp_reply = is_arp_reply;
          t->sender_ip    = sender_ip;
          t->target_ip    = target_ip;
          clib_memcpy (t->sender_mac, sender_mac, 6);
        }

      vlib_set_next_frame_buffer (vm, node, next, bi);

      from++;
      n_left--;
    }

  return frame->n_vectors;
}

 //Init: store my_ip in network byte order once at startup
static clib_error_t *
myarp_init (vlib_main_t *vm)
{
  my_ip = clib_host_to_net_u32 (0x0a0a0101);  /* 10.10.1.1 */
  return 0;
}

VLIB_INIT_FUNCTION (myarp_init);

 // Node registration  — now includes format_trace
VLIB_REGISTER_NODE (myarp_node) = {
  .name          = "myarp-node",
  .function      = myarp_node_fn,
  .format_trace  = format_myarp_trace,   /* enables: trace add myarp-node N */
  .vector_size   = sizeof (u32),
  .n_next_nodes  = MYARP_N_NEXT,
  .next_nodes    = {
    [MYARP_NEXT_DROP]         = "error-drop",
    [MYARP_NEXT_IFACE_OUTPUT] = "interface-output",
  },
};

/* ----------------------------------------------------------------
 * Feature registration
 * arc : device-input — intercepts every raw inbound packet
 *       before ANY VPP built-in logic runs
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
