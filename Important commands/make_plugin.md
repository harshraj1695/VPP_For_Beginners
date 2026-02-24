# VPP Plugin Build Notes (From Step 2 Onward)

## Step 2 --- Create Plugin Folder

``` bash
cd ~/vpp/src/plugins
mkdir myplugin
cd myplugin
```

------------------------------------------------------------------------

## Step 3 --- Create Required Files

### Plugin Source File

Create:

``` bash
nano myplugin.c
```

``` c
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
```

------------------------------------------------------------------------

### CMake File

``` bash
nano CMakeLists.txt
```

``` cmake
add_vpp_plugin(myplugin
  SOURCES
  myplugin.c
)
```

------------------------------------------------------------------------

### API File (required even if empty)

``` bash
touch myplugin.api
```

------------------------------------------------------------------------

## Step 4 --- Build Plugin

Always build from repo root:

``` bash
cd ~/vpp
make wipe
make build-release
```

------------------------------------------------------------------------

## Step 5 --- Verify Plugin Compiled

``` bash
find ~/vpp/build-root -name "*myplugin*"
```

Expected output should include:

    myplugin_plugin.so

------------------------------------------------------------------------

## Step 6 --- Run VPP

``` bash
make run-release
```

------------------------------------------------------------------------

## Step 7 --- Test Plugin

Inside VPP CLI:

``` bash
hello
```

Or externally:

``` bash
./build-root/install-vpp-native/vpp/bin/vppctl hello
```

Expected output:

    Hello from my first VPP plugin!

------------------------------------------------------------------------

## Errors Encountered and Fixes

### Wrong source filename in CMake

**Cause:** referenced `main.c` instead of `myplugin.c`\
**Fix:** update CMakeLists to use correct filename.

------------------------------------------------------------------------

### Ran CMake command in shell

**Cause:** typed `add_vpp_plugin(...)` in terminal\
**Fix:** put it inside `CMakeLists.txt`.

------------------------------------------------------------------------

### Deprecated macro error

**Cause:** used `VPP_BUILD_VER`\
**Fix:** replace with string version:

``` c
.version = "1.0"
```

------------------------------------------------------------------------

### Plugin not appearing

**Cause:** VPP not restarted after build\
**Fix:**

``` bash
make run-release
```

------------------------------------------------------------------------

### Tried compiling plugin with gcc

**Cause:** treated plugin like standalone C program\
**Fix:** plugins must be built using:

``` bash
make build-release
```
