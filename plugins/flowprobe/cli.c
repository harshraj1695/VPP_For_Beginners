#include <vlib/vlib.h>
#include "flowprobe.h"

static clib_error_t *
flowprobe_enable_disable(vlib_main_t *vm,
                         unformat_input_t *input,
                         vlib_cli_command_t *cmd)
{
    flowprobe_main.enabled = 1;
    return 0;
}

VLIB_CLI_COMMAND(flowprobe_enable_command) = {
    .path = "flowprobe enable",
    .short_help = "Enable flow probe",
    .function = flowprobe_enable_disable,
};

static clib_error_t *
flowprobe_show(vlib_main_t *vm,
               unformat_input_t *input,
               vlib_cli_command_t *cmd)
{
    vlib_cli_output(vm,
        "Dropped records: %lu",
        flowprobe_main.dropped);
    return 0;
}

VLIB_CLI_COMMAND(flowprobe_show_command) = {
    .path = "show flowprobe",
    .short_help = "Show flowprobe stats",
    .function = flowprobe_show,
};