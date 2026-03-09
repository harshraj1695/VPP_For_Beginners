# VPP Custom ICMP Plugin

A custom VPP graph node that handles ICMP Echo Requests and replies with a custom payload signature. All non-ICMP packets are dropped. No predefined VPP ICMP nodes are used.

---

## What It Does

```
ping 10.10.1.1
  └─► virtio-input
        └─► ethernet-input
              └─► ip4-input → ip4-lookup → ip4-receive
                    └─► ip4-local arc
                          └─► [icmp-input]  ← our custom node
                                │
                                ├─ ICMP Echo Request to 10.10.1.1
                                │     → swap MACs
                                │     → swap IPs
                                │     → type 8 → 0 (Echo Reply)
                                │     → stamp "harsh has done it... " in payload
                                │     → recompute checksums
                                │     └─► interface-output → tap0-tx
                                │
                                └─ EVERYTHING ELSE
                                      └─► error-drop
```

---

## File Structure

```
icmp_custom/
├── icmp.h          — structs, defines, ICMP header layout
├── icmp_node.c     — graph node, checksum, trace, feature registration
└── CMakeLists.txt  — build config
```

---

## How the Node Works

### Packet Decision Logic

```c
if (ip->protocol != ICMP)          → DROP
if (ip->dst != 10.10.1.1)          → DROP
if (icmp->type != Echo Request)    → DROP

// else: build reply in-place
swap MACs (ethernet layer)
swap src/dst IPs
recompute IP checksum
set ICMP type = 0 (Echo Reply)
write "harsh has done it... " at payload[16]   // skip ping timestamp
recompute ICMP checksum
send out same interface
```

### Why payload offset 16?

`ping` stores its send timestamp at bytes 0–15 of the ICMP payload to calculate RTT. Writing at offset 16 preserves the timestamp so RTT stays accurate and no warnings are thrown.

---

## Build

### 1. Copy plugin into VPP source tree

```bash
cp -r icmp_custom/ $VPP_SRC/src/plugins/
```

### 2. Build

```bash
cd $VPP_SRC
make build
```

---

## Setup & Run

### Configure VPP

```
vpp# create tap id 0 host-if-name vpp-tap0
vpp# set interface state tap0 up
vpp# set interface ip address tap0 10.10.1.1/24
vpp# set interface feature tap0 icmp-input arc ip4-local
```

### Configure host

```bash
sudo ip addr add 10.10.1.2/24 dev vpp-tap0
sudo ip link set vpp-tap0 up
```

### Ping

```bash
ping 10.10.1.1
```

Expected output:
```
64 bytes from 10.10.1.1: icmp_seq=1 ttl=64 time=0.264 ms
64 bytes from 10.10.1.1: icmp_seq=2 ttl=64 time=0.087 ms
```

---

## Verify Custom Payload

```bash
sudo tcpdump -i vpp-tap0 -A icmp
```

Every reply will contain the custom signature:

```
10.10.1.1 > 10.10.1.2: ICMP echo reply
E..T..@.@.....
....harsh has done it... ...........
```

---

## Trace

```
vpp# trace add virtio-input 20
vpp# show trace
```

```
virtio-input
  ethernet-input
    ip4-input
      ip4-lookup
        ip4-receive
          icmp-input: 10.10.1.1 -> 10.10.1.2 type=0 seq=1 action=REPLIED [harsh has done it... ]
            tap0-output
              tap0-tx
```

For dropped packets (non-ICMP):
```
icmp-input: 10.10.1.2 -> 224.0.0.22 type=148 seq=63746 action=DROPPED
  error-drop
```

---

## Key Implementation Details

| Detail | Value |
|--------|-------|
| Node name | `icmp-input` |
| Feature arc | `ip4-local` |
| Runs before | `ip4-icmp-input` (intercepts before VPP built-in) |
| Node IP | `10.10.1.1` |
| Reply next node | `interface-output` |
| Drop next node | `error-drop` |
| Custom payload | `"harsh has done it... "` at payload offset 16 |
| Checksum | Ones-complement, computed from scratch on every reply |

---

## Proof of Working

```
12:23:21 IP 10.10.1.1 > LAP-63648: ICMP echo reply, id 12879, seq 1, length 64
E..T..@.@.....
....harsh has done it... ........... !"#$%&'()*+,-./01234567

12:23:22 IP 10.10.1.1 > LAP-63648: ICMP echo reply, id 12879, seq 2, length 64
E..T!.@.@.....
....harsh has done it... ........... !"#$%&'()*+,-./01234567
```
