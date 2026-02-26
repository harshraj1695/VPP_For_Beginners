#include <vlib/vlib.h>
#include <vlib/unix/plugin.h>

static clib_error_t *
hi_one_command_fn (vlib_main_t * vm,
                   unformat_input_t * input,
                   vlib_cli_command_t * cmd)
{
    u32 number;

    if (unformat(input, "%u", &number))
    {
        vlib_cli_output(vm, "You entered number: %u", number);
    }
    else
    {
        return clib_error_return(0,
                "Please provide a number. Example: hi one 10");
    }

    return 0;
}

/* Register CLI command */
VLIB_CLI_COMMAND (hi_one_command, static) = {
    .path = "hi one",
    .short_help = "hi one <number>",
    .function = hi_one_command_fn,
};

/* Register plugin */
VLIB_PLUGIN_REGISTER () = {
    .version = "1.0",
    .description = "My Second Plugin",
};