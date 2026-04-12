# wio-bg770a-water-sensor-interrupt

Wio BG770A に接続した Grove Water Sensor の Digital ピンを `CHANGE` 割り込みで監視し、`HIGH/LOW` の変化を検知したときだけ通知するサンプルです。

[センサー情報](https://wiki.seeedstudio.com/Grove-Water_Sensor/)

## 動作

- Grove コネクタ P1 の `D28` を `CHANGE` 割り込みで監視します。
- Digital 入力が `HIGH` から `LOW`、または `LOW` から `HIGH` に切り替わったときだけシリアル出力します。
- 常時 `loop()` で値をポーリングせず、状態変化時だけ通知します。
- HIGH/LOW のデジタル変化のみを扱い、Analog 値は読み取りません。

## シリアル出力例

```text
Wio BG770A Water Sensor Interrupt Sample
Grove P1 digital input: D28(28)
Interrupt mode: detect digital HIGH/LOW changes
[startup] digital=HIGH(1)
[digital-change] digital=LOW(0)
[digital-change] digital=HIGH(1)
```

## 実装ポイント

- `attachInterrupt(..., CHANGE)` で Digital 入力の HIGH/LOW 変化を検知します。
- 割り込みハンドラではフラグ更新のみを行い、実際の `Serial` 出力は `loop()` 側で実行します。
- `loop()` 側では前回出力値と比較し、同じ値の連続出力だけを抑えます。
- 起動時に一度だけ現在値を読み取り、初期状態を表示します。
