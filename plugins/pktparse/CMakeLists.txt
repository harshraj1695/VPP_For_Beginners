add_vpp_plugin(pktparse
  SOURCES
  pktparse.c

  LINK_LIBRARIES
  vppinfra
  vlib
  vnet
)
