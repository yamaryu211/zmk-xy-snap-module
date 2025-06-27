# ZMK XY Snap Input Processor

ZMK 用の XY 軸ロック（スナップ）input-processor モジュールです。

## 特徴

- トラックボールやタッチパッドの XY 軸を自動でロックし、直線的なスクロールや操作を実現
- デバイストリー（DTS）から柔軟に設定可能
- ZMK 外部モジュールとして公式流儀で組み込み可能

## ディレクトリ構成

```
zmk-xy-snap-module/
├── CMakeLists.txt
├── Kconfig
├── README.md
├── dts/
│   ├── xy_snap_input_processor.dtsi
│   └── bindings/
│       └── input-processor/
│           └── zmk,xy-snap-input-processor.yaml
├── include/
│   └── zmk_xy_snap_input_processor.h
├── src/
│   └── zmk_xy_snap_input_processor.c
└── zephyr/
    └── module.yml
```

## 導入方法

### 1. west.yml に追加

```yaml
- name: zmk-xy-snap-module
  path: modules/zmk-xy-snap-module
  url: <YOUR_REPO_URL>
  revision: main
```

### 2. `zephyr/module.yml`で自動認識

### 3. `-DZMK_EXTRA_MODULES=modules/zmk-xy-snap-module` をビルド時に指定

### 4. devicetree で input-processor を有効化

```dts
#include <xy_snap_input_processor.dtsi>

&trackball_listener {
    input-processors = <&xy_snap_input_processor>;
};
```

## 設定例（DTSI）

```dts
xy_snap_input_processor: xy_snap_input_processor {
    compatible = "zmk,xy-snap-input-processor";
    #input-processor-cells = <0>;
    idle-timeout-ms = <200>;
    switch-threshold = <20>;
    allow-axis-switch;
};
```

## Kconfig オプション

- `CONFIG_ZMK_XY_SNAP_IDLE_TIMEOUT_MS` : ロック解除までの無入力時間（ms）
- `CONFIG_ZMK_XY_SNAP_SWITCH_THRESHOLD` : 軸切り替えの閾値
- `CONFIG_ZMK_XY_SNAP_ALLOW_AXIS_SWITCH` : 軸切り替え許可

## ビルド例

```sh
west build -s zmk/app -b <board> -- -DZMK_EXTRA_MODULES=modules/zmk-xy-snap-module
```

## ライセンス

MIT
