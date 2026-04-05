# wio-bg770a-energy-saving

Wio BG770A から任意の HTTP endpoint へ定期送信しながら、PSM、Idle、モデム電源 OFF の待機方法による消費電力差を比較するコードです。

送信時の通信状態と利用中プロファイルをあわせて記録し、省電力設定ごとの挙動を観測するためのサンプルが入っています。送信自体は HTTP POST で `echo.getpostman.com/post` に対して行います。

送信先は [`src/unified_endpoint.cpp`](src/unified_endpoint.cpp) の `ENDPOINT_HOST` / `ENDPOINT_PORT` / `ENDPOINT_PATH` を変更してください。SIM 接続に必要な APN と認証情報は [`src/main.cpp`](src/main.cpp) の `APN` / `PDP_AUTH_*` を変更してください。

現在の送信間隔は `src/power_profile_config.cpp` の `SEND_INTERVAL_MS` で 5 分に設定しています。消費電力測定では 30 分だと差が見えにくく、10 分よりも短い条件で PSM の効果を観測しやすいため、比較用の初期値として 5 分を採用しています。

## Power profile modes

このファームウェアには 3 つの比較モードが入っています。

1. `psm` (`profile id = 3`)
送信後に PSM への移行を試みるモードです。PSM に入れなかった場合も、そのままモジュール電源 OFF には落とさず、PSM 失敗として観測できます。

2. `idle` (`profile id = 1`)
送信後もモジュールを起動したまま在圏維持するモードです。次周期でもモデムの再起動を行わず、そのまま通信を継続します。

3. `poweroff` (`profile id = 2`)
毎回送信後にモデム電源を完全に OFF にするモードです。再送信時に毎回起動と再接続をやり直します。

`profile id` は消費電力の大きい順に振っています。

- `1`: `idle`
- `2`: `poweroff`
- `3`: `psm`

## Mode switching without reflashing

モード選択はファームウェアに保存され、再起動後も維持されます。

- 電源投入時にユーザーボタンを約 1.5 秒長押しすると、モードが次へ進みます。
- 長押しするたびに `profile id` 順で `1 -> 2 -> 3 -> 1` と切り替わります。
- LED はまず約 2 秒間の長点灯を 1 回行い、少し間を空けてから `profile id` と同じ回数だけ 1 秒周期で点滅します。
- 起動直後に、現在モードを同じ回数だけ点滅表示します。
- 1 回点滅: `idle`
- 2 回点滅: `poweroff`
- 3 回点滅: `psm`

USB シリアル接続は不要なので、PPK2 給電のまま切り替えできます。
