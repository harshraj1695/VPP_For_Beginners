add_vpp_plugin(packet_count
  SOURCES
  node.c

  LINK_LIBRARIES
  vppinfra
  vlib
  vnet
)
