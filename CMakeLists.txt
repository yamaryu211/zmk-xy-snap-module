# Zephyrモジュールとしての設定
zephyr_module()

# モジュールが有効な場合のみビルド
if(CONFIG_ZMK_XY_SNAP_MODULE)
  # ライブラリの設定
  zephyr_library()
  zephyr_library_sources(src/zmk_xy_snap_input_processor.c)
  zephyr_include_directories(include)
endif()