# CANReceiver (簡易仕様)

## 目的
`CANReceiver` クラスは、CAN バスから受信した `ENGINE_DATA` (ID=123) フレームを最小限にデコードし、`ENGINE_Torque` と `ENGINE_RPM` を出力する軽量受信クラスです。

## 公開 API
- `CANReceiver()`
  - コンストラクタ。CAN ライブラリを初期化し、チャネルをオープンしてバスを開始します。
- `~CANReceiver()`
  - デストラクタ。バスを停止してチャネルを閉じます。
- `void run()`
  - 無限ループで `canRead()` をポーリングし、`ENGINE_DATA` フレームを見つけたらデコードして値をコンソールに出力します。

## 内部動作（簡易）
- 初期化: `canInitializeLibrary()`, `canOpenChannel(0, canOPEN_ACCEPT_VIRTUAL)`
- バス設定: `canSetBusParams(h, 500000, ...)`, `canBusOn(h)`
- 受信ループ: `canRead()` をポーリングしてメッセージが得られたら処理
- デコード:
  - bytes 0-1 -> ENGINE_Torque (unsigned16, little-endian) → torque = raw * 0.1
  - bytes 2-3 -> ENGINE_RPM (unsigned16, little-endian) → rpm = raw * 0.25
- ログ: 最小限の標準出力（`ENGINE_DATA: Torque=X Nm, RPM=Y`）

## 設計方針
- エラー処理は最小限（失敗時はログを出すが例外を投げない）
- シンプルで堅牢なポーリング実装
- マルチスレッドや割り込みの複雑性は排除

## 制約
- 受信は `ENGINE_DATA`（ID=123）のデコードのみを目的とする
- 高度なメッセージフィルタ、タイムスタンプ整合、メッセージバッファリングは未実装

## ファイル
- 実装: `can_receiver.cpp`

## 使用例
```cpp
CANReceiver r;
r.run();
```

## テストシナリオ
1. `can_sender` を別プロセスで起動して ENGINE_DATA フレームを送信
2. `can_receiver` を起動し、受信ログに `ENGINE_DATA: Torque=... Nm, RPM=...` が出ることを確認

## 既知の制限
- Ctrl+C によるグレースフルシャットダウンは簡易実装ではサポートしていない（必要なら拡張する）
- CAN チャネルのエラーコード詳細は表示しない

## 将来拡張候補
- メッセージフィルタリング（ID 列挙）
- ログ出力のフォーマット選択（CSV/JSON）
- Graceful shutdown の追加
- DBC を用いた動的デコード
