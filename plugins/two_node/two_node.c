
#include <vlib/vlib.h>
#include <vnet/vnet.h>
#include <vnet/plugin/plugin.h>


int nodea=0;
int nodeb=0;
typedef enum {
    TWO_NODE_NEXT_NODEB,
    TWO_NODE_NEXT_IP4,
    TWO_NODE_N_NEXT,
} two_node_next_t;



static uword
nodeA_fn(vlib_main_t *vm,
         vlib_node_runtime_t *node,
         vlib_frame_t *frame)
{
    u32 *from = vlib_frame_vector_args(frame);
    u32 n_left = frame->n_vectors;
    u16 nexts[VLIB_FRAME_SIZE];

    // clib_warning("NodeA running, packets: %u", n_left);
     nodea+=n_left;
    for (u32 i = 0; i < n_left; i++)
    {
        nexts[i] = TWO_NODE_NEXT_NODEB;  // send to nodeB
    }

    vlib_buffer_enqueue_to_next(vm, node, from, nexts, n_left);
    return n_left;
}

VLIB_REGISTER_NODE(nodeA) = {
    .name = "nodeA",
    .function = nodeA_fn,
    .vector_size = sizeof(u32),
    .type = VLIB_NODE_TYPE_INTERNAL,

    .n_next_nodes = TWO_NODE_N_NEXT,   // MUST match enum count

    .next_nodes = {
        [TWO_NODE_NEXT_NODEB] = "nodeB",
        [TWO_NODE_NEXT_IP4]   = "ip4-lookup",
    },
};



static uword
nodeB_fn(vlib_main_t *vm,
         vlib_node_runtime_t *node,
         vlib_frame_t *frame)
{
    u32 *from = vlib_frame_vector_args(frame);
    u32 n_left = frame->n_vectors;
    u16 nexts[VLIB_FRAME_SIZE];

    // clib_warning("NodeB running, packets: %u", n_left);
    nodeb+=n_left;
    for (u32 i = 0; i < n_left; i++)
    {
        nexts[i] = 0;  // only one next node → ip4-lookup
    }

    vlib_buffer_enqueue_to_next(vm, node, from, nexts, n_left);
    return n_left;
}

VLIB_REGISTER_NODE(nodeB) = {
    .name = "nodeB",
    .function = nodeB_fn,
    .vector_size = sizeof(u32),
    .type = VLIB_NODE_TYPE_INTERNAL,

    .n_next_nodes = 1,

    .next_nodes = {
        [0] = "ip4-lookup",
    },
};

// CLI to show packet counts at nodeB
static clib_error_t *
show_pktcounter_fn (vlib_main_t * vm,
                    unformat_input_t * input,
                    vlib_cli_command_t * cmd)
{
    vlib_cli_output(vm, "Packets seen at nodeA: %d", nodea);
    vlib_cli_output(vm, "Packets seen at nodeB: %d", nodeb);
    return 0;
}

VLIB_CLI_COMMAND (show_pktcounter_cmd, static) = {
    .path = "show twonode",
    .short_help = "show twonode - Display packet counts at nodeA and nodeB",
    .function = show_pktcounter_fn,
};
VNET_FEATURE_INIT (two_node_feature, static) = {
    .arc_name = "ip4-unicast",
    .node_name = "nodeA",
    .runs_before = VNET_FEATURES ("ip4-lookup"),
};


VLIB_PLUGIN_REGISTER() = {
    .version = "1.0",
    .description = "Two Node Feature Plugin",
};