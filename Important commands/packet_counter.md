
# VPP Packet Counter Plugin — Complete Working Notes

## 1️⃣ Create plugin directory
```bash
cd ~/vpp/src/plugins
mkdir pktcounter
cd pktcounter
```

---

## 2️⃣ Create CMakeLists.txt
```cmake
add_vpp_plugin(pktcounter
  SOURCES
  pktcounter.c
)
```

---

## 3️⃣ Create pktcounter.c (WORKING VERSION)

```c
#include <vlib/vlib.h>
#include <vlib/unix/plugin.h>
#include <vnet/vnet.h>
#include <vnet/ip/ip.h>

typedef struct {
    u64 packet_count;
} pktcounter_main_t;

pktcounter_main_t pktcounter_main;

static uword
pktcounter_node_fn (vlib_main_t * vm,
                    vlib_node_runtime_t * node,
                    vlib_frame_t * frame)
{
    u32 n_packets = frame->n_vectors;
    pktcounter_main.packet_count += n_packets;
    return n_packets;
}

VLIB_REGISTER_NODE(pktcounter_node) = {
    .name = "pktcounter-node",
    .function = pktcounter_node_fn,
    .vector_size = sizeof(u32),
    .type = VLIB_NODE_TYPE_INTERNAL,
};

VNET_FEATURE_INIT(pktcounter_feat, static) = {
    .arc_name = "ip4-unicast",
    .node_name = "pktcounter-node",
    .runs_before = VNET_FEATURES("ip4-lookup"),
};

static clib_error_t *
show_pktcounter_fn (vlib_main_t * vm,
                    unformat_input_t * input,
                    vlib_cli_command_t * cmd)
{
    vlib_cli_output(vm, "Packets seen: %lu",
        pktcounter_main.packet_count);
    return 0;
}

VLIB_CLI_COMMAND (show_pktcounter_cmd, static) = {
    .path = "show pktcounter",
    .short_help = "show pktcounter",
    .function = show_pktcounter_fn,
};

VLIB_PLUGIN_REGISTER () = {
    .version = "1.0",
    .description = "Simple packet counter plugin",
};
```

---

## 4️⃣ Build VPP
```bash
cd ~/vpp
make build-release
```

---

## 5️⃣ Run VPP
```bash
make run-release
```

---

## 6️⃣ Create loopback interface inside VPP
```bash
create loopback interface
set interface state loop0 up
set interface ip address loop0 10.1.1.1/24
```

---

## 7️⃣ Attach packet counter feature
```bash
set interface feature loop0 pktcounter-node arc ip4-unicast
```

---

## 8️⃣ Generate traffic inside VPP
```bash
ping 10.1.1.1
```

---

## 9️⃣ See packet counter
```bash
show pktcounter
```

---

# Optional TAP test

```bash
create tap id 0 host-if-name vpp-tap0
set interface state tap0 up
set interface ip address tap0 10.10.1.1/24
set interface feature tap0 pktcounter-node arc ip4-unicast
```

Linux side:
```bash
sudo ip addr add 10.10.1.2/24 dev vpp-tap0
sudo ip link set vpp-tap0 up
ping 10.10.1.1
```

Back in VPP:
```bash
show pktcounter
```

---

# Result
Your plugin now counts real dataplane packets inside VPP.
