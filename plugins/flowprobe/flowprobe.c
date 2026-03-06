#include <vnet/plugin/plugin.h>
#include "flowprobe.h"

flowprobe_main_t flowprobe_main;

static clib_error_t *
flowprobe_init(vlib_main_t *vm)
{
    flowprobe_main_t *fm = &flowprobe_main;

    fm->n_workers = vlib_num_workers();
    fm->rings = clib_mem_alloc(
        sizeof(flow_ring_t) * (fm->n_workers + 1));

    for (int i = 0; i <= fm->n_workers; i++)
    {
        fm->rings[i].records =
            clib_mem_alloc(sizeof(flow_record_t) *
                           FLOW_RING_SIZE);

        fm->rings[i].head = 0;
        fm->rings[i].tail = 0;
    }

    fm->enabled = 0;
    fm->dropped = 0;

    return 0;
}

VLIB_INIT_FUNCTION(flowprobe_init);

VLIB_PLUGIN_REGISTER () = {
    .version = "1.0",
    .description = "Flow Probe Text File Exporter",
};