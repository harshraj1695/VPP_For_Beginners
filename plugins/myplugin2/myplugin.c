#include <vlib/vlib.h>
#include <vlib/unix/plugin.h>

/* ---- Internal functions ---- */

static void print1_fn(vlib_main_t *vm)
{
    vlib_cli_output(vm, "Print1 function executed");
}

static void print2_fn(vlib_main_t *vm)
{
    vlib_cli_output(vm, "Print2 function executed");
}

static void print3_fn(vlib_main_t *vm)
{
    vlib_cli_output(vm, "Print3 function executed");
}

// cli command function
static clib_error_t *
hi_two_command_fn (vlib_main_t * vm,
                   unformat_input_t * input,
                   vlib_cli_command_t * cmd)
{
    if (unformat(input, "print1"))
    {
        print1_fn(vm);
    }
    else if (unformat(input, "print2"))
    {
        print2_fn(vm);
    }
    else if (unformat(input, "print3"))
    {
        print3_fn(vm);
    }
    else
    {
        return clib_error_return(0,
            "Usage: hi two print1|print2|print3");
    }

    return 0;
}

// Register CLI command

VLIB_CLI_COMMAND (hi_two_command, static) = {
    .path = "hi two",
    .short_help = "hi two print1|print2|print3",
    .function = hi_two_command_fn,
};

// Register plugin
VLIB_PLUGIN_REGISTER () = {
    .version = "1.0",
    .description = "Multiple print function CLI example",
};