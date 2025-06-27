zephyr_library()
zephyr_library_sources(src/xy_snap_input_processor.c)
zephyr_include_directories(include)

# DTS bindingsディレクトリを追加
zephyr_include_directories(dts) 