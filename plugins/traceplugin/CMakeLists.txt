add_vpp_plugin(traceplugin
  SOURCES
  node.c

  LINK_LIBRARIES
  vppinfra
  vlib
  vnet
)
