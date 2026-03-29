# KVASER CAN クラス仕様書

## 目次
1. [CANSender クラス](#cansender-クラス)
2. [CANReceiver クラス](#canreceiver-クラス)
3. [使用例](#使用例)
4. [注意点](#注意点)

---

## CANSender クラス

### 概要
`CANSender` クラスは、KVASER CANLIB を使用して CAN バスにメッセージを送信する機能を提供します。クラスはコンストラクタでリソースの初期化を行い、デストラクタで自動的にクリーンアップします（RAII パターン）。

### クラス定義ファイル
`can_sender.cpp`

### 定数（private static constexpr）

| 定数名 | 型 | 値 | 説明 |
|--------|----|----|------|
| `CAN_ID` | `long` | `0x100` | 送信する CAN メッセージの ID |
| `PERIOD_MS` | `unsigned int` | `1000` | メッセージ送信周期（ミリ秒） |
| `DATA_LENGTH` | `unsigned int` | `8` | CAN メッセージのデータ長（バイト） |

### メンバ変数（private）

| 変数名 | 型 | 説明 |
|--------|----|----|
| `h` | `canHandle` | CANLIB のチャネルハンドル |
| `messageCount` | `int` | 送信したメッセージのカウント |

### メンバ関数

#### public

##### `CANSender()`
**コンストラクタ**

初期化リスト：
```cpp
h(-1), messageCount(0)
```

処理内容：
- CANLIB ライブラリの初期化
- CAN チャネルのオープン
- CAN バスパラメータの設定
- CAN バスの起動

**例外安全性**：基本的保証

---

##### `~CANSender()`
**デストラクタ**

処理内容：
- CAN バスの停止
- CAN チャネルのクローズ
- リソースの解放

**例外安全性**：強い保証（例外を発生させない）

---

##### `void run()`
**CAN メッセージ送信メインループ**

処理内容：
1. 送信データバッファを初期化（全て 0x00）
2. ユーザーへの通知メッセージを表示
3. 無限ループで以下を繰り返す：
   - CAN メッセージを送信キューに追加
   - メッセージカウントをインクリメント
   - 送信情報をコンソールに出力
   - `PERIOD_MS` ミリ秒待機

**出力例**：
```
CAN transmission start (Ctrl+C to exit)

CAN ID: 0x100, Period: 1 second

[1] Sent: ID=0x100, Data=[00 00 00 00 00 00 00 00]
[2] Sent: ID=0x100, Data=[00 00 00 00 00 00 00 00]
...
```

**備考**：無限ループのため、プログラムは Ctrl+C で終了

---

#### private

##### `void initializeChannel()`
CAN チャネルを開く

CANLIB API: `canOpenChannel()`
- チャネル番号：0
- フラグ：`canOPEN_ACCEPT_VIRTUAL`（仮想チャネル受け入れ）

---

##### `void setBusParams()`
CAN バスの速度とタイミングを設定

CANLIB API: `canSetBusParams()`
- バス速度：500,000 bps（500 kbps）
- タイミングパラメータ：自動設定（0, 0, 0）

---

##### `void startBus()`
CAN バスを有効にして通信開始

CANLIB API: `canBusOn()`

---

##### `void stopBus()`
CAN バスを無効にして通信停止

CANLIB API: `canBusOff()`

---

## CANReceiver クラス

### 概要
`CANReceiver` クラスは、KVASER CANLIB を使用して CAN バスからメッセージを受信する機能を提供します。クラスはコンストラクタでリソースの初期化を行い、デストラクタで自動的にクリーンアップします（RAII パターン）。

### クラス定義ファイル
`can_receiver.cpp`

### 定数（private static constexpr）

| 定数名 | 型 | 値 | 説明 |
|--------|----|----|------|
| `DATA_LENGTH` | `unsigned int` | `8` | CAN メッセージの最大データ長（バイト） |

### メンバ変数（private）

| 変数名 | 型 | 説明 |
|--------|----|----|
| `h` | `canHandle` | CANLIB のチャネルハンドル |
| `messageCount` | `int` | 受信したメッセージのカウント |

### メンバ関数

#### public

##### `CANReceiver()`
**コンストラクタ**

初期化リスト：
```cpp
h(-1), messageCount(0)
```

処理内容：
- CANLIB ライブラリの初期化
- CAN チャネルのオープン
- CAN バスパラメータの設定
- CAN バスの起動

**例外安全性**：基本的保証

---

##### `~CANReceiver()`
**デストラクタ**

処理内容：
- CAN バスの停止
- CAN チャネルのクローズ
- リソースの解放

**例外安全性**：強い保証（例外を発生させない）

---

##### `void run()`
**CAN メッセージ受信メインループ**

処理内容：
1. ローカル変数を初期化（メッセージ ID、データバッファ、DLC など）
2. ユーザーへの通知メッセージを表示
3. 無限ループで以下を繰り返す：
   - `readMessage()` でメッセージ受信を試行
   - 100 ミリ秒待機

**出力例**：
```
CAN reception start (Ctrl+C to exit)

Waiting for CAN messages...

[1] Received: ID=0x100, DLC=8, Data=[00 00 00 00 00 00 00 00]
[2] Received: ID=0x100, DLC=8, Data=[00 00 00 00 00 00 00 00]
...
```

**備考**：無限ループのため、プログラムは Ctrl+C で終了

---

#### private

##### `void initializeChannel()`
CAN チャネルを開く

CANLIB API: `canOpenChannel()`
- チャネル番号：0
- フラグ：`canOPEN_ACCEPT_VIRTUAL`（仮想チャネル受け入れ）

---

##### `void setBusParams()`
CAN バスの速度とタイミングを設定

CANLIB API: `canSetBusParams()`
- バス速度：500,000 bps（500 kbps）
- タイミングパラメータ：自動設定（0, 0, 0）

---

##### `void startBus()`
CAN バスを有効にして通信開始

CANLIB API: `canBusOn()`

---

##### `void stopBus()`
CAN バスを無効にして通信停止

CANLIB API: `canBusOff()`

---

##### `void readMessage(long *id, unsigned char *msg, unsigned int *dlc, unsigned int *flags, unsigned long *time_stamp)`

**CAN メッセージを受信キューから読み込み**

パラメータ（出力ポインタ）：
| パラメータ | 型 | 説明 |
|-----------|----|----|
| `id` | `long*` | 受信したメッセージ ID |
| `msg` | `unsigned char*` | 受信データバッファ（最大 8 バイト） |
| `dlc` | `unsigned int*` | 受信データ長 |
| `flags` | `unsigned int*` | メッセージフラグ |
| `time_stamp` | `unsigned long*` | タイムスタンプ（ミリ秒単位） |

CANLIB API: `canRead()`

処理内容：
- `canStatus` コードで受信状態を判定
- 受信成功時（`canOK`）：
  - メッセージカウントをインクリメント
  - メッセージ情報をコンソールに出力（ID、DLC、データを 16 進数表示）

---

## 使用例

### CANSender の使用例

```cpp
#include "can_sender.cpp"

int main() {
    CANSender sender;
    sender.run();  // メッセージ送信を開始（無限ループ）
    return 0;
}
```

**実行結果**：
```
CAN transmission start (Ctrl+C to exit)

CAN ID: 0x100, Period: 1 second

[1] Sent: ID=0x100, Data=[00 00 00 00 00 00 00 00]
[2] Sent: ID=0x100, Data=[00 00 00 00 00 00 00 00]
...（1 秒ごとに送信）
```

### CANReceiver の使用例

```cpp
#include "can_receiver.cpp"

int main() {
    CANReceiver receiver;
    receiver.run();  // メッセージ受信を開始（無限ループ）
    return 0;
}
```

**実行結果**：
```
CAN reception start (Ctrl+C to exit)

Waiting for CAN messages...

[1] Received: ID=0x100, DLC=8, Data=[00 00 00 00 00 00 00 00]
[2] Received: ID=0x100, DLC=8, Data=[00 00 00 00 00 00 00 00]
...
```

### 同時実行例

**ターミナル 1（送信）**：
```bash
C:\...> can_sender.exe
CAN transmission start (Ctrl+C to exit)
CAN ID: 0x100, Period: 1 second
[1] Sent: ID=0x100, Data=[00 00 00 00 00 00 00 00]
```

**ターミナル 2（受信）**：
```bash
C:\...> can_receiver.exe
CAN reception start (Ctrl+C to exit)
Waiting for CAN messages...
[1] Received: ID=0x100, DLC=8, Data=[00 00 00 00 00 00 00 00]
```

---

## 注意点

### リソース管理
- **RAII パターン**を採用：コンストラクタで初期化、デストラクタで自動クリーンアップ
- デストラクタが不意に呼ばれないよう、例外処理に注意が必要

### バス速度
- 両クラスとも **500 kbps** で固定
- 異なるバス速度で通信する場合は、定数値を変更後に再コンパイル

### チャネル管理
- チャネル 0 を使用（`canOpenChannel(0, ...)`）
- 複数デバイス/チャネルを使用する場合は、クラスのパラメータ化が必要

### 仮想チャネル
- `canOPEN_ACCEPT_VIRTUAL` フラグで仮想チャネルを受け入れ
- 実デバイスでない場合、両プログラムは同じ仮想バス上で通信

### メッセージ送受信
- **CANSender**：常に 0x00 データを送信（カスタマイズ可能）
- **CANReceiver**：受信メッセージが存在しない場合、100 ms ごとにポーリング

### エラーハンドリング
- 現在の実装にはエラー処理が最小限
- 本番環境では、CANLIB API の戻り値をチェックして例外処理を追加すること

### ポーリング間隔
- **CANReceiver** は 100 ms ごとにメッセージをチェック
- より低遅延が必要な場合は、この値を調整

---

## 変数スコープ

### 初期化時の状態

| クラス | 変数 | 初期値 |
|--------|------|--------|
| `CANSender` | `h` | `-1` |
| `CANSender` | `messageCount` | `0` |
| `CANReceiver` | `h` | `-1` |
| `CANReceiver` | `messageCount` | `0` |

コンストラクタ実行後、`h` は有効なハンドル値（正の整数）に更新されます。

---

## 関連 API リファレンス

### KVASER CANLIB API

| 関数 | 説明 |
|------|------|
| `canInitializeLibrary()` | CANLIB 初期化 |
| `canOpenChannel(int channel, int flags)` | チャネルオープン |
| `canSetBusParams(...)` | バスパラメータ設定 |
| `canBusOn(canHandle h)` | バス起動 |
| `canBusOff(canHandle h)` | バス停止 |
| `canWrite(...)` | メッセージ送信 |
| `canRead(...)` | メッセージ受信 |
| `canClose(canHandle h)` | チャネルクローズ |

詳細は `canlib.chm` を参照してください。
