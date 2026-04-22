# VPP + veth Pair + Network Namespace Setup

A complete guide to creating a veth pair connecting a Linux network namespace (`vpp-ns`) to a VPP (Vector Packet Processing) instance running in the default namespace.

---

## 📐 Architecture

```
DEFAULT NAMESPACE                        vpp-ns NAMESPACE
─────────────────────────────────        ────────────────────────
  veth0  (linux interface, no IP)          veth1
    │                                        │
    │  AF_PACKET socket                      │
    ▼                                        │
  host-veth0 (VPP interface)                 │
  IP: 10.0.0.1/24  ◄════════════════►  IP: 10.0.0.2/24
                      veth pair
                    (virtual cable)
```

### Key Concepts
- **VPP** runs in the **default namespace** and attaches to `veth0` via an AF_PACKET socket
- **veth0** exists in the default namespace but has **no IP at the Linux kernel level** — VPP owns 10.0.0.1 in userspace
- **veth1** lives inside `vpp-ns` namespace, managed by Linux kernel with IP 10.0.0.2
- The veth pair acts as a **virtual cable** between VPP and the namespace

---

## 🛠️ Prerequisites

- Linux machine (Ubuntu 20.04 / 22.04 recommended)
- VPP installed and built from source or via package
- `iproute2` installed (provides `ip` command)
- Root / sudo access

---

## 📦 Part 1 — Linux Side Setup

### Step 1 — Create the veth Pair

```bash
sudo ip link add veth0 type veth peer name veth1
```

| Part | Explanation |
|------|-------------|
| `ip link add` | Create a new network interface |
| `veth0` | First end of the virtual cable (stays in default namespace, used by VPP) |
| `type veth` | Interface type is virtual ethernet |
| `peer name veth1` | Second end of the cable (will go into the namespace) |

> A veth pair is like a virtual network cable — whatever enters one end exits the other.

---

### Step 2 — Bring Up Both Interfaces

```bash
sudo ip link set veth0 up
sudo ip link set veth1 up
```

| Command | Explanation |
|---------|-------------|
| `ip link set veth0 up` | Activate veth0 in default namespace |
| `ip link set veth1 up` | Activate veth1 before moving it to namespace |

> Interfaces are created in DOWN state by default. `up` enables them so traffic can flow.

### Verify

```bash
ip link show | grep veth
```

Expected output:
```
6: veth1@veth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 ...
7: veth0@veth1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 ...
```

---

### Step 3 — Create the Network Namespace

```bash
sudo ip netns add vpp-ns
```

| Part | Explanation |
|------|-------------|
| `ip netns add` | Create a new isolated network namespace |
| `vpp-ns` | Name of the namespace (just a label) |

> A network namespace is a completely isolated copy of the Linux network stack — its own interfaces, routing table, iptables rules, and ARP table.

---

### Step 4 — Move veth1 into the Namespace

```bash
sudo ip link set veth1 netns vpp-ns
```

| Part | Explanation |
|------|-------------|
| `ip link set veth1` | Target the veth1 interface |
| `netns vpp-ns` | Move it into the vpp-ns namespace |

> After this command, `veth1` disappears from the default namespace and only exists inside `vpp-ns`.

---

### Step 5 — Configure veth1 Inside the Namespace

All commands prefixed with `sudo ip netns exec vpp-ns` run **inside** the `vpp-ns` namespace:

```bash
# Bring up veth1
sudo ip netns exec vpp-ns ip link set veth1 up

# Bring up loopback interface
sudo ip netns exec vpp-ns ip link set lo up

# Assign IP address to veth1
sudo ip netns exec vpp-ns ip addr add 10.0.0.2/24 dev veth1
```

| Command | Explanation |
|---------|-------------|
| `ip netns exec vpp-ns` | Execute the following command inside vpp-ns namespace |
| `ip link set veth1 up` | Activate veth1 inside the namespace |
| `ip link set lo up` | Activate loopback — needed for localhost to work inside namespace |
| `ip addr add 10.0.0.2/24 dev veth1` | Assign IP 10.0.0.2 with /24 subnet to veth1 |

> `/24` means subnet mask `255.255.255.0` — covers the range `10.0.0.1` to `10.0.0.254`.
> Adding an IP also auto-creates a kernel route: `10.0.0.0/24 dev veth1`.

### Verify

```bash
sudo ip netns exec vpp-ns ip addr show
```

Expected output:
```
1: lo: <LOOPBACK,UP,LOWER_UP> ...
    inet 127.0.0.1/8 scope host lo
6: veth1@if7: <BROADCAST,MULTICAST,UP,LOWER_UP> ...
    inet 10.0.0.2/24 scope global veth1
```

---

## 🚀 Part 2 — VPP Side Setup

Start VPP (from source build):

```bash
cd vpp
make run-release
```

Or if installed via package:

```bash
sudo vppctl
```

---

### Step 6 — Attach VPP to veth0 via AF_PACKET

```
vpp# create host-interface name veth0
```

| Part | Explanation |
|------|-------------|
| `create host-interface` | Create an AF_PACKET interface — attaches to a Linux interface |
| `name veth0` | The Linux interface to attach to (must exist in default namespace) |
| Output: `host-veth0` | VPP's internal name — always prefixes `host-` to AF_PACKET interfaces |

> VPP opens a raw AF_PACKET socket on `veth0`. All packets on `veth0` now flow directly
> into VPP's packet processing graph, bypassing the Linux kernel network stack entirely.

---

### Step 7 — Assign IP Address to VPP Interface

```
vpp# set interface ip address host-veth0 10.0.0.1/24
```

| Part | Explanation |
|------|-------------|
| `set interface ip address` | Assign an IP to a VPP interface |
| `host-veth0` | The VPP interface name |
| `10.0.0.1/24` | IP address — VPP owns this in userspace |

> **Important:** This IP is managed by VPP in userspace, NOT by the Linux kernel.
> The Linux kernel does NOT know about 10.0.0.1 — `ping 10.0.0.1` from the host will fail,
> but it works from inside `vpp-ns` because packets travel through the veth pair into VPP.

---

### Step 8 — Bring Up the VPP Interface

```
vpp# set interface state host-veth0 up
```

| Part | Explanation |
|------|-------------|
| `set interface state` | Change the state of a VPP interface |
| `host-veth0` | Which interface |
| `up` | Activate it so VPP can send/receive packets |

---

### Step 9 — Test Connectivity

From VPP, ping the namespace:

```
vpp# ping 10.0.0.2
```

Expected output:
```
116 bytes from 10.0.0.2: icmp_seq=2 ttl=64 time=4.09 ms
116 bytes from 10.0.0.2: icmp_seq=3 ttl=64 time=5.95 ms
116 bytes from 10.0.0.2: icmp_seq=4 ttl=64 time=13.14 ms
Statistics: 4 sent, 3 received, 25% packet loss
```

> The 25% packet loss on first run is **normal** — seq=1 is lost due to ARP resolution delay.
> Run `ping 10.0.0.2` again and you will get 0% packet loss.

From the namespace, ping VPP:

```bash
sudo ip netns exec vpp-ns ping 10.0.0.1
```

Expected output:
```
64 bytes from 10.0.0.1: icmp_seq=1 ttl=64 time=1.22 ms
64 bytes from 10.0.0.1: icmp_seq=2 ttl=64 time=1.23 ms
```

---

## 🧹 Cleanup

```bash
# Delete namespace (also removes veth1 inside it)
sudo ip netns del vpp-ns

# Delete veth0 from host (also removes its peer)
sudo ip link del veth0
```

---

## 📋 All Commands — Quick Reference

### Linux Side
```bash
sudo ip link add veth0 type veth peer name veth1
sudo ip link set veth0 up
sudo ip link set veth1 up
sudo ip netns add vpp-ns
sudo ip link set veth1 netns vpp-ns
sudo ip netns exec vpp-ns ip link set veth1 up
sudo ip netns exec vpp-ns ip link set lo up
sudo ip netns exec vpp-ns ip addr add 10.0.0.2/24 dev veth1

# Verify
sudo ip netns exec vpp-ns ip addr show
sudo ip netns exec vpp-ns ip route show
```

### VPP Side (inside vpp# CLI)
```
create host-interface name veth0
set interface ip address host-veth0 10.0.0.1/24
set interface state host-veth0 up
ping 10.0.0.2
```

---

## 🔍 Useful VPP Debug Commands

```
# Show all interfaces and their state
vpp# show interface

# Show IP addresses assigned in VPP
vpp# show interface address

# Show VPP routing table
vpp# show ip fib

# Show ARP table
vpp# show arp

# Capture and trace packets (run BEFORE pinging)
vpp# trace add af-packet-input 10
vpp# show trace

# Show hardware interface details
vpp# show hardware-interfaces

# Show memif interfaces (if using memif)
vpp# show memif
```

---

## 💡 Key Takeaways

| Concept | Detail |
|---------|--------|
| `veth0` in default namespace | No IP at kernel level — VPP owns it via AF_PACKET socket |
| `host-veth0` in VPP | VPP's view of veth0 — IP 10.0.0.1 managed in userspace |
| `veth1` in vpp-ns | Linux kernel manages this — IP 10.0.0.2 |
| VPP bypasses kernel | Packets go userspace → VPP graph, not through Linux netstack |
| AF_PACKET | Raw socket type VPP uses to read/write packets directly on veth0 |
| 25% packet loss | Normal on first ping — caused by ARP resolution on seq=1 |

---

## 📚 Further Reading

- [VPP fd.io Documentation](https://fd.io/documentation/)
- [VPP Source Code](https://github.com/FDio/vpp)
- [Linux Network Namespaces](https://man7.org/linux/man-pages/man7/network_namespaces.7.html)
- [AF_PACKET socket man page](https://man7.org/linux/man-pages/man7/packet.7.html)
