# VPP Control‑Plane Programming Notes (After Debugging)

## What type of programming is this?

This is **Control‑Plane Programming** for VPP.

There are two main ways to program VPP:

### 1. Dataplane Programming

-   Done using **VPP Plugins**
-   Runs inside VPP
-   Processes packets directly
-   Written in C
-   Built with VPP build system

### 2. Control‑Plane Programming ✅ (what you just did)

-   Runs **outside VPP**
-   Talks to VPP through the Binary API socket
-   Used by automation systems, routers, orchestration tools
-   Can be written in Python, Go, or C
-   Safer and easier to debug than plugins

You successfully implemented **external control‑plane programming**.

------------------------------------------------------------------------

# Steps to Build a Working Python Control‑Plane Client

## Step 1 --- Ensure VPP is running

``` bash
cd ~/vpp
make run-release
```

Leave this terminal open.

------------------------------------------------------------------------

## Step 2 --- Activate Python virtual environment

``` bash
cd ~/VPP_Pratice/sample
source vppenv/bin/activate
```

------------------------------------------------------------------------

## Step 3 --- Export the correct VPP Python bindings

``` bash
export PYTHONPATH=$HOME/vpp/build-root/install-vpp-native/vpp/local/lib/python3.12/dist-packages
```

------------------------------------------------------------------------

## Step 4 --- Confirm API JSON directory exists

``` bash
ls ~/vpp/build-root/install-vpp-native/vpp/share/vpp/api
```

You should see:

    core/
    plugins/

------------------------------------------------------------------------

# Working Programs (Final Debugged Versions)

------------------------------------------------------------------------

# Program 1 --- Test connection and list interfaces

Save as:

``` bash
nano test_vpp.py
```

``` python
from vpp_papi import VPPApiClient

vpp = VPPApiClient(apidir=[
    "/home/harshraj1695/vpp/build-root/install-vpp-native/vpp/share/vpp/api"
])

vpp.connect("python-client")
print("Connected to VPP!")

for i in vpp.api.sw_interface_dump():
    print("Interface:", i.interface_name)

vpp.disconnect()
print("Disconnected.")
```

Run:

``` bash
python test_vpp.py
```

Expected output:

    Connected to VPP!
    Interface: local0
    Disconnected.

------------------------------------------------------------------------

# Program 2 --- Create Loopback Interface

Save as:

``` bash
nano create_loopback.py
```

``` python
from vpp_papi import VPPApiClient

vpp = VPPApiClient(apidir=[
    "/home/harshraj1695/vpp/build-root/install-vpp-native/vpp/share/vpp/api"
])

vpp.connect("python-client")
print("Connected to VPP!")

# Create loopback
reply = vpp.api.create_loopback_instance(
    mac_address=b"\x02\x00\x00\x00\x00\x01"
)
sw_if_index = reply.sw_if_index

print("Created loopback with index:", sw_if_index)

# Bring interface up
vpp.api.sw_interface_set_flags(
    sw_if_index=sw_if_index,
    flags=1
)

print("\nInterfaces after creation:")
for i in vpp.api.sw_interface_dump():
    print("Interface:", i.interface_name)

vpp.disconnect()
print("Disconnected.")
```

Run:

``` bash
python create_loopback.py
```

Expected output:

    Connected to VPP!
    Created loopback with index: 1

    Interfaces after creation:
    Interface: local0
    Interface: loop0
    Disconnected.

------------------------------------------------------------------------

# Minimal Workflow to Remember

    1. Start VPP
    2. Activate venv
    3. Export PYTHONPATH
    4. Run Python client

------------------------------------------------------------------------

# What you can do next

You now have full control‑plane access.

Next possible programs:

-   Assign IP address to interface
-   Add route
-   Configure ACL
-   Control your plugin
-   Build automation scripts
