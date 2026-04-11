# wio-bg770a-water-sensor-polling

Wio BG770A に接続した Grove Water Sensor を `analogRead()` で周期的に読み取り、しきい値ベースで `WET/DRY` を判定してシリアル出力するサンプルです。

[センサー情報](https://wiki.seeedstudio.com/Grove-Water_Sensor/)

## 動作

- Grove P1 の `A4` を 1 秒ごとに読み取ります。
- `analog` 値が `700` 未満なら `WET`、以上なら `DRY` と判定します。
- このサンプルは Analog 専用です。Digital 値は読み取りません。

## シリアル出力例

```text
Wio BG770A + Grove Water Sensor
Grove Analog P1: analog=A4(28), wet_threshold=700
Lower analog value means wetter
analog=907 threshold=700 state=DRY
analog=645 threshold=700 state=WET
```
