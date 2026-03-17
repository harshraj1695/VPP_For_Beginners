# VPP Practice

This repository is a general-purpose VPP learning workspace.
It collects small experiments, custom plugins, control-plane samples, and setup notes that make it easier to learn how VPP is built, extended, and controlled.

The repo is useful if you want to:

- learn basic VPP CLI and developer workflows
- build small custom plugins and graph nodes
- experiment with packet parsing, tracing, counters, and ARP/ICMP logic
- try simple control-plane integrations from C or Python
- keep personal notes and working examples in one place

## Repository Layout

### `plugins/`

Hands-on VPP plugin experiments and node-level dataplane work.

Examples in this repo include:

- `myplugin`, `myplugin2`, `cmd_parse`, `traceplugin`
- `pktparse`, `pktcounter`, `packet_count`
- `myarp`, `icmp_custom`
- `two_node`, `tracedump1`, `flowprobe`

These folders are good for learning:

- how a minimal VPP plugin is structured
- how to register CLI commands
- how to add or test custom nodes
- how to inspect packets and traces
- how to experiment with feature arcs and forwarding behavior

### `Important_commands/`

Reference notes and short guides for recurring VPP tasks.

Current topics include:

- VPP installation
- plugin creation
- veth and VPP setup
- packet counter notes
- ARP notes
- control-plane notes

This folder is the quickest place to look when you need working commands instead of theory.

### `Control_Pannel/`

Small control-plane side experiments for talking to VPP from user space.

- `python_sample/` shows a basic Python `vpp_papi` connection
- `sample_c/` contains a small C example for interface-related work

This area is helpful when moving from VPP CLI usage to API-driven automation.

## Suggested Learning Path

If you are new to VPP, a simple order is:

1. Read [Important_commands/vpp_install.md](/home/harshraj1695/VPP_Pratice/Important_commands/vpp_install.md) to get a development setup running.
2. Read [Important_commands/make_plugin.md](/home/harshraj1695/VPP_Pratice/Important_commands/make_plugin.md) to understand the minimum plugin structure.
3. Explore simple plugins such as `plugins/myplugin/`, `plugins/myplugin2/`, and `plugins/cmd_parse/`.
4. Move to packet-focused experiments like `pktparse`, `pktcounter`, `packet_count`, and `traceplugin`.
5. Study richer examples like [plugins/icmp_custom/README.md](/home/harshraj1695/VPP_Pratice/plugins/icmp_custom/README.md) once the basics feel comfortable.
6. Try the control-plane examples in `Control_Pannel/` to connect to VPP programmatically.

## How To Use This Repo

This repo is not a single buildable product by itself.
Think of it as a practice notebook plus example collection for VPP development.

Depending on what you are learning, you will usually:

- copy or adapt a plugin into a local VPP source tree
- build VPP from the VPP source root
- run VPP and test behavior with `vppctl`, traces, pings, or traffic generators
- use the notes in `Important_commands/` as a quick reference

## Good Starting Points

- Minimal plugin structure: `plugins/myplugin/`
- Command parsing and CLI experiments: `plugins/cmd_parse/`
- Packet counting: `plugins/packet_count/`, `plugins/pktcounter/`
- Packet parsing: `plugins/pktparse/`
- Custom ICMP handling: `plugins/icmp_custom/`
- Python API usage: `Control_Pannel/python_sample/test_vpp.py`

## Who This Repo Is For

This repository is intended for:

- beginners learning VPP internals
- developers practicing custom plugin development
- anyone who wants a personal sandbox for VPP dataplane and control-plane experiments

## Notes

- Some examples are intentionally small and learning-oriented rather than production-ready.
- Folder names and experiments reflect iterative practice work.
- You may need to adjust absolute paths, interface names, or local VPP build locations for your machine.

## Goal

The goal of this repository is simple:
learn VPP by building small, understandable things and keeping the working notes close to the code.
