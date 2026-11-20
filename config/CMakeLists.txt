zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ZMK_BEHAVIOR_NORDIC_DFU
  src/behaviors/behavior_nordic_dfu.c
)
