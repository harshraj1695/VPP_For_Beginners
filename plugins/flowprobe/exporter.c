#include <vlib/vlib.h>
#include <arpa/inet.h>
#include "flowprobe.h"

static uword
flowprobe_exporter_process(vlib_main_t *vm,
                           vlib_node_runtime_t *rt,
                           vlib_frame_t *f)
{
    flowprobe_main_t *fm = &flowprobe_main;

    fm->fp = fopen("/tmp/flowprobe.txt", "a");
    setvbuf(fm->fp, NULL, _IOFBF, 4<<20);

    while (1)
    {
        vlib_process_wait_for_event_or_clock(vm, 0.5);

        for (int i = 0; i <= fm->n_workers; i++)
        {
            flow_ring_t *ring = &fm->rings[i];

            while (ring->tail != ring->head)
            {
                flow_record_t *r =
                    &ring->records[ring->tail];

                struct in_addr s, d;
                s.s_addr = r->src;
                d.s_addr = r->dst;

                fprintf(fm->fp,
                        "%s %s %u %.6f\n",
                        inet_ntoa(s),
                        inet_ntoa(d),
                        r->bytes,
                        r->timestamp);

                ring->tail =
                    (ring->tail + 1) % FLOW_RING_SIZE;
            }
        }

        fflush(fm->fp);
    }

    return 0;
}

VLIB_REGISTER_NODE(flowprobe_exporter_node) = {
    .function = flowprobe_exporter_process,
    .type = VLIB_NODE_TYPE_PROCESS,
    .name = "flowprobe-exporter",
};