# ZMK XY Snap Input Processor

ZMK 用の XY 軸ロック（スナップ）input-processor モジュールです。**iPhone のような「縦方向に動き出したら横方向には動かない」スクロール挙動**を実現します。

## 特徴

- **iPhone 風の方向ロック**: トラックボールの動き出し方向を検出し、その方向にロックして直線的なスクロールを実現
- **累積値による初期方向決定**: 小さな動きを蓄積して、より正確な初期方向を判定
- **軸切り替え制御**: 意図しない軸の切り替えを防止し、安定したスクロール体験を提供
- デバイストリー（DTS）から柔軟に設定可能
- ZMK 外部モジュールとして公式流儀で組み込み可能

## iPhone 風の挙動について

このモジュールは、iPhone のタッチスクロールで見られる以下の挙動を再現します：

1. **初期方向の決定**: トラックボールを動かし始めた方向（縦または横）を検出
2. **方向ロック**: 検出された方向にロックし、他の方向の動きを無視
3. **安定したスクロール**: ロック中は一方向のみのスクロールが継続
4. **自動リセット**: 一定時間無操作後、ロックが自動解除

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
│   ├── zmk_xy_snap_input_processor.c
│   └── zmk_xy_snap_input_behavior.c
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

### 3. GitHub Actions でビルド

### 4. devicetree で input-processor を有効化

```dts
#include "../modules/zmk-xy-snap-module/dts/xy_snap_input_processor.dtsi"

&trackball_listener {
    input-processors = <&xy_snap_input_processor>;
};
```

## 設定例（DTSI）

### iPhone 風の挙動（推奨設定）

```dts
xy_snap: xy_snap {
    compatible = "zmk,xy-snap-input-processor";
    #input-processor-cells = <0>;

    // iPhone風の挙動に最適化されたパラメータ
    idle-timeout-ms = <300>;        // より長いタイムアウト
    switch-threshold = <50>;        // より高い切り替え閾値
    initial-threshold = <10>;       // 初期方向決定の閾値
    // allow-axis-switch;           // 軸切り替えを無効化（コメントアウト）
    status = "okay";
};
```

### 従来の挙動（軸切り替え可能）

```dts
xy_snap: xy_snap {
    compatible = "zmk,xy-snap-input-processor";
    #input-processor-cells = <0>;

    idle-timeout-ms = <200>;
    switch-threshold = <20>;
    initial-threshold = <5>;
    allow-axis-switch;              // 軸切り替えを有効化
    status = "okay";
};
```

## パラメータ詳細

### `idle-timeout-ms`

- **説明**: 無入力で軸ロックを解除するまでのタイムアウト（ミリ秒）
- **デフォルト**: 300ms
- **推奨値**: iPhone 風の挙動では 300ms 以上

### `switch-threshold`

- **説明**: 軸切り替えを許可する閾値
- **デフォルト**: 50
- **推奨値**: iPhone 風の挙動では 50 以上（または軸切り替えを無効化）

### `initial-threshold`

- **説明**: 初期方向決定のための累積移動量の閾値
- **デフォルト**: 10
- **推奨値**: 5-15 の範囲で調整

### `allow-axis-switch`

- **説明**: 軸切り替えを許可するかどうか
- **デフォルト**: false（iPhone 風の挙動）
- **推奨値**: iPhone 風の挙動では無効化

## Kconfig オプション

- `CONFIG_ZMK_XY_SNAP_IDLE_TIMEOUT_MS` : ロック解除までの無入力時間（ms）
- `CONFIG_ZMK_XY_SNAP_SWITCH_THRESHOLD` : 軸切り替えの閾値
- `CONFIG_ZMK_XY_SNAP_INITIAL_THRESHOLD` : 初期方向決定の閾値
- `CONFIG_ZMK_XY_SNAP_ALLOW_AXIS_SWITCH` : 軸切り替え許可

## 使用例

### keymap.dtsi での使用例

```dts
trackball_listener {
    status = "okay";
    compatible = "zmk,input-listener";
    device = <&trackball>;

    scroller_mac {
        layers = <FUNC_SCROLL_MAC>;
        input-processors =
            <&zip_xy_scaler 1 8>,
            <&zip_xy_transform INPUT_TRANSFORM_Y_INVERT>,
            <&xy_snap>,                    // ← ここで方向ロックを挿入
            <&zip_xy_to_scroll_mapper>;
    };
};
```

## ライセンス

MIT
