#include <vlib/vlib.h>
#include <vlib/unix/plugin.h>

static clib_error_t *
hello_command_fn (vlib_main_t * vm,
                  unformat_input_t * input,
                  vlib_cli_command_t * cmd)
{
    vlib_cli_output(vm, "Hello from my first VPP plugin!");
    return 0;
}

VLIB_CLI_COMMAND (hello_command, static) = {
    .path = "hello",
    .short_help = "hello",
    .function = hello_command_fn,
};

VLIB_PLUGIN_REGISTER () = {
    .version = "1.0",
    .description = "My First Plugin",
};
