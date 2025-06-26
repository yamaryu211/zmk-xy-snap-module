# zmk-xy-snap-module

## 概要

トラックボールのスクロールを XY 軸いずれか一方向に固定し、斜め方向のスクロールを防止する ZMK 用 input-processor モジュールです。

## 主な機能

- 動き出し時に X/Y 軸の大きい方に軸固定
- 軸固定中は他軸方向の移動を無視（または反対軸方向の移動量が閾値を超えた場合のみ軸切り替え可）
- 停止後、一定時間（デフォルト 200ms）で軸固定解除
- パラメータは zmk-config で設定可能
- input-processors で指定可能

## パラメータ（Kconfig で設定）

- `ZMK_XY_SNAP_IDLE_TIMEOUT_MS` : 無入力解除タイムアウト（デフォルト: 200ms）
- `ZMK_XY_SNAP_SWITCH_THRESHOLD` : 軸切り替え閾値（デフォルト: 20）
- `ZMK_XY_SNAP_ALLOW_AXIS_SWITCH` : 軸切り替え許可モード（デフォルト: 有効）

## west.yml への追加例（zmk-config 側）

zmk-config リポジトリの`west.yml`に以下を追加してください。

```yaml
manifest:
  remotes:
    - name: yamaryu211
      url-base: https://github.com/yamaryu211
  projects:
    - name: zmk-xy-snap-module
      remote: yamaryu211
      revision: main
      path: modules/zmk-xy-snap-module
  self:
    path: config
```

追加後、以下のコマンドで取得できます。

```sh
west update
```

## 設定例（zmk-config 側）

```conf
CONFIG_ZMK_XY_SNAP_IDLE_TIMEOUT_MS=200
CONFIG_ZMK_XY_SNAP_SWITCH_THRESHOLD=20
CONFIG_ZMK_XY_SNAP_ALLOW_AXIS_SWITCH=y
```

## 使用方法

### 1. DTSI ファイルをインクルード

keymap ファイル（例: `roBa.keymap`）の先頭に以下を追加：

```dts
#include <dts/xy_snap_input_processor.dtsi>
```

### 2. input-processors で指定

trackball_listener の input-processors に追加：

```dts
trackball_listener {
    status = "okay";
    compatible = "zmk,input-listener";
    device = <&trackball>;

    input-processors = <&xy_snap_input_processor>;
};
```

### 3. パラメータのカスタマイズ（オプション）

DTSI ファイル内でパラメータを変更可能：

```dts
xy_snap_input_processor: xy_snap_input_processor {
    compatible = "zmk,xy-snap-input-processor";
    #input-processor-cells = <0>;

    idle-timeout-ms = <300>;        // 300msに変更
    switch-threshold = <15>;        // 15に変更
    allow-axis-switch;              // 軸切り替え許可
};
```

## ライセンス

MIT
