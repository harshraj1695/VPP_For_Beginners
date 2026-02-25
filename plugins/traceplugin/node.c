#include <vlib/vlib.h>
#include <vnet/vnet.h>
#include <vnet/ip/ip.h>
#include <vlib/unix/plugin.h>

/* -------- TRACE STRUCT -------- */

typedef struct
{
  u32 sw_if_index;
  u16 pkt_len;
} traceplug_trace_t;

/* -------- GLOBAL STATE -------- */

typedef struct
{
  u64 packets;
} traceplug_main_t;

traceplug_main_t traceplug_main;

/* -------- TRACE FORMATTER -------- */

static u8 *
format_traceplug_trace (u8 * s, va_list * args)
{
  CLIB_UNUSED (vlib_main_t * vm) = va_arg (*args, vlib_main_t *);
  CLIB_UNUSED (vlib_node_t * node) = va_arg (*args, vlib_node_t *);
  traceplug_trace_t * t = va_arg (*args, traceplug_trace_t *);

  s = format (s, "traceplug: sw_if_index %u len %u",
              t->sw_if_index, t->pkt_len);
  return s;
}

/* -------- NODE FUNCTION -------- */

VLIB_NODE_FN (traceplug_node) (vlib_main_t * vm,
                               vlib_node_runtime_t * node,
                               vlib_frame_t * frame)
{
  u32 *from, *to_next;
  u32 n_left_from, n_left_to_next;
  u32 next_index = 0;

  from = vlib_frame_vector_args (frame);
  n_left_from = frame->n_vectors;

  while (n_left_from > 0)
  {
    vlib_get_next_frame (vm, node, next_index,
                         to_next, n_left_to_next);

    while (n_left_from > 0 && n_left_to_next > 0)
    {
      u32 bi0 = from[0];
      vlib_buffer_t *b0 = vlib_get_buffer (vm, bi0);

      /* count packet */
      traceplug_main.packets++;

      /* trace support */
      if (PREDICT_FALSE (b0->flags & VLIB_BUFFER_IS_TRACED))
      {
        traceplug_trace_t *t =
          vlib_add_trace (vm, node, b0, sizeof (*t));

        t->sw_if_index =
          vnet_buffer (b0)->sw_if_index[VLIB_RX];

        t->pkt_len =
          vlib_buffer_length_in_chain (vm, b0);
      }

      /* forward packet to next node */
      to_next[0] = bi0;

      from++;
      to_next++;
      n_left_from--;
      n_left_to_next--;
    }

    vlib_put_next_frame (vm, node,
                         next_index, n_left_to_next);
  }

  return frame->n_vectors;
}
/* -------- NODE REGISTRATION -------- */

VLIB_REGISTER_NODE (traceplug_node) = {
  .name = "traceplug",
  .vector_size = sizeof (u32),
  .format_trace = format_traceplug_trace,
  .type = VLIB_NODE_TYPE_INTERNAL,

  .n_next_nodes = 1,
  .next_nodes = {
    [0] = "ip4-lookup",
  },
};

/* -------- FEATURE ARC INSERTION -------- */

#include <vnet/feature/feature.h>

VNET_FEATURE_INIT (traceplug_feature, static) = {
  .arc_name = "ip4-unicast",
  .node_name = "traceplug",
  .runs_before = VNET_FEATURES ("ip4-lookup"),
};

/* -------- CLI COMMAND -------- */

static clib_error_t *
show_traceplug_fn (vlib_main_t * vm,
                   unformat_input_t * input,
                   vlib_cli_command_t * cmd)
{
  vlib_cli_output (vm, "Packets seen: %lu",
                   traceplug_main.packets);
  return 0;
}

VLIB_CLI_COMMAND (show_traceplug, static) = {
  .path = "show traceplug",
  .short_help = "show traceplug",
  .function = show_traceplug_fn,
};
VLIB_PLUGIN_REGISTER () = {
    .version = "1.0",
    .description = "My First Plugin",
};
