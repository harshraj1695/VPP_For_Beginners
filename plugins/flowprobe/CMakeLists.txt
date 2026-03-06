add_vpp_plugin(flowprobe
  SOURCES
    flowprobe.c
    node.c
    exporter.c
    cli.c

  INSTALL_HEADERS
    flowprobe.h
)