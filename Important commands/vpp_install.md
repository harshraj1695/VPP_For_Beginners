# VPP Developer Installation Guide

------------------------------------------------------------------------

## STEP 0 --- Install build prerequisites

Run this first:

``` bash
sudo apt update
sudo apt install -y build-essential git cmake ninja-build pkg-config
sudo apt install -y python3 python3-pip python3-setuptools python3-wheel
sudo apt install -y libnuma-dev libpcap-dev libssl-dev
sudo apt install -y clang llvm
sudo apt install -y curl wget
```

------------------------------------------------------------------------

## STEP 1 --- Clone VPP source code

``` bash
git clone https://github.com/FDio/vpp.git
cd vpp
```

Optional but recommended stable branch:

``` bash
git checkout stable/2506
```

------------------------------------------------------------------------

## STEP 2 --- Install all VPP dependencies (automatic)

Official method:

``` bash
make install-dep
```

This may take **10--20 minutes**.\
It installs:

-   DPDK dependencies\
-   build tools\
-   libraries\
-   python modules

------------------------------------------------------------------------

## STEP 3 --- Build VPP (developer build)

``` bash
make build-release
```

First build takes **15--40 minutes depending on CPU**.

When done, you will have:

    build-root/install-vpp-native/vpp/

That folder contains:

-   vpp binary\
-   headers\
-   libraries\
-   plugins\
-   API bindings

------------------------------------------------------------------------

## STEP 4 --- Run VPP from source

``` bash
make run-release
```

You should see:

    vpp#

That means VPP started successfully.

------------------------------------------------------------------------

## STEP 5 --- Test CLI connectivity

Open another terminal:

``` bash
cd vpp
./build-root/install-vpp-native/vpp/bin/vppctl show version
```

If it prints version → ✔ working.

------------------------------------------------------------------------

## STEP 6 --- Install Python API bindings (for programming)

``` bash
cd src/vpp-api/python
pip3 install .
```

------------------------------------------------------------------------

## STEP 7 --- Test Python control program

Create file:

``` bash
nano test_vpp.py
```

Paste:

``` python
from vpp_papi import VPP

vpp = VPP(use_socket=True)
vpp.connect("test-client")

interfaces = vpp.api.sw_interface_dump()

for i in interfaces:
    print(i.interface_name, i.sw_if_index)

vpp.disconnect()
```

Run:

``` bash
python3 test_vpp.py
```

If interfaces print → ✔ API working.

------------------------------------------------------------------------

## STEP 8 --- Confirm dataplane dev works

Modify a file:

``` bash
nano src/vnet/ip/ip4_input.c
```

Add anywhere inside packet loop:

``` c
clib_warning("PACKET HIT IP4 INPUT");
```

Rebuild:

``` bash
make build-release
make run-release
```

Send traffic → you'll see log messages.

✔ That confirms you can program dataplane.

------------------------------------------------------------------------

# DONE --- Full VPP Dev Environment Ready

You can now:

-   write plugins\
-   modify packet pipeline\
-   create API controllers\
-   add new nodes\
-   build custom forwarding logic
