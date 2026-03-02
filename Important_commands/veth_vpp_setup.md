# Create veth Pair in Linux

``` bash
sudo ip link add veth0 type veth peer name veth1
```

## Bring both interfaces UP

``` bash
sudo ip link set veth0 up
sudo ip link set veth1 up
```

## Verify

``` bash
ip link show | grep veth
```

------------------------------------------------------------------------

# 2️⃣ Assign IP to Linux Side

``` bash
sudo ip addr add 10.10.1.1/24 dev veth0
```

## Check

``` bash
ip addr show veth0
```

------------------------------------------------------------------------

# 3️⃣ Create Host Interface in VPP (for veth1)

``` bash
create host-interface name veth1
```

## Bring it up

``` bash
set interface state host-veth1 up
```

## Assign IP inside VPP

``` bash
set interface ip address host-veth1 10.10.1.2/24
```

## Verify

``` bash
show interface address
```

------------------------------------------------------------------------

# 4️⃣ Test From Linux

``` bash
ping 10.10.1.2
```

------------------------------------------------------------------------

# 5️⃣ Delete / Cleanup

## Delete veth pair from Linux

``` bash
sudo ip link delete veth0
```

## Delete interface from VPP

``` bash
delete host-interface name veth1
```
