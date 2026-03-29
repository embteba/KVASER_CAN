# CAN Sender プログラム仕様書

## 1. 概要

**プログラム名**: CAN Sender  
**バージョン**: 1.0  
**説明**: KVASER CANlib を使用して、CAN バスに ENGINE_DATA メッセージを周期的に送信するプログラム

---

## 2. 機能

### 主要機能
- CAN チャネルの初期化と管理
- ENGINE_DATA メッセージの定期送信（1秒間隔）
- ENGINE_RPM と ENGINE_Torque シグナルのエンコーディング
- エラー検出とハンドリング
- 送信フレームカウンティング

### サポートされるプラットフォーム
- Windows x64
- KVASER Canlib ライブラリ対応デバイス

---

## 3. 技術仕様

### 3.1 CAN メッセージ定義

| 項目 | 値 |
|------|-----|
| メッセージ名 | ENGINE_DATA |
| メッセージID | 0x7B (123 decimal) |
| データ長 | 8 バイト |
| 送信周期 | 1000 ms（1秒） |
| バス速度 | 500 kbps |

### 3.2 シグナル定義

#### ENGINE_Torque (エンジントルク)
| パラメータ | 値 |
|-----------|-----|
| スタートビット | 0 |
| ビット長 | 16 bits |
| バイト位置 | 0-1 (Little-Endian) |
| 単位 | Nm (ニュートンメートル) |
| スケール | 0.1 Nm/unit |
| オフセット | 0 |
| 最小値 | 0 Nm |
| 最大値 | 6553.5 Nm |

#### ENGINE_RPM (エンジン回転数)
| パラメータ | 値 |
|-----------|-----|
| スタートビット | 16 |
| ビット長 | 16 bits |
| バイト位置 | 2-3 (Little-Endian) |
| 単位 | rpm (回転/分) |
| スケール | 0.25 rpm/unit |
| オフセット | 0 |
| 最小値 | 0 rpm |
| 最大値 | 16383.75 rpm |

### 3.3 CAN フレームレイアウト

```
バイト位置:  0      1      2      3      4      5      6      7
┌────────┬────────┬────────┬────────┬────────┬────────┬────────┬────────┐
│ TORQUE │ TORQUE │  RPM   │  RPM   │  RES   │  RES   │  RES   │  RES   │
│(LSB)   │(MSB)   │(LSB)   │(MSB)   │        │        │        │        │
└────────┴────────┴────────┴────────┴────────┴────────┴────────┴────────┘

TORQUE: ENGINE_Torque (16-bit, Little-Endian)
RPM: ENGINE_RPM (16-bit, Little-Endian)
RES: Reserved (0x00)
```

---

## 4. ハードウェア要件

### 必須
- Windows PC (x64 プロセッサ)
- KVASER CAN デバイス（以下のいずれか）
  - KVASER USBcan シリーズ
  - KVASER Virtual CAN
  - その他 KVASER Canlib 対応デバイス

### ソフトウェア
- KVASER Canlib (bin_x64 以上)
- Visual C++ ランタイム
- DLL:
  - `canlib32.dll` (CAN API)
  - 実行ファイルと同じディレクトリに配置

---

## 5. プログラム構成

### 5.1 クラス構成

```
CANSender
├── private:
│   ├── 定数パラメータ（CAN_ID、PERIOD_MS など）
│   ├── メンバ変数（h, messageCount）
│   ├── initializeChannel() - CAN チャネル初期化
│   ├── setBusParams() - バスパラメータ設定
│   ├── startBus() - CAN バス開始
│   └── stopBus() - CAN バス停止
├── public:
│   ├── CANSender() - コンストラクタ
│   ├── ~CANSender() - デストラクタ
│   └── run() - メインループ（送信処理）
```

### 5.2 初期化フロー

1. **ライブラリ初期化**: `canInitializeLibrary()`
2. **チャネルオープン**: `canOpenChannel(0, canOPEN_ACCEPT_VIRTUAL)`
3. **バス設定**: `canSetBusParams(h, 500000, ...)`
4. **バス開始**: `canBusOn(h)`
5. **メッセージ送信ループ開始**

### 5.3 送信フロー

```
each 1000ms:
├── Torque値をスケール（0.1）で正規化
├── RPM値をスケール（0.25）で正規化
├── CAN フレームバッファを構築
│   ├── Torque → bytes[0-1]
│   ├── RPM → bytes[2-3]
│   └── Reserved → bytes[4-7]
├── canWrite() で CAN バスに送信
└── 送信情報をコンソール出力
```

---

## 6. 使用方法

### 6.1 コンパイル

```bash
# Visual Studio 2022 の開発者コマンドプロンプトで実行
cd c:\SourceCode\C++\KVASER\KVASER_CAN_

# MSVC コンパイラでコンパイル
cl.exe can_sender.cpp ^
  /I"C:\Program Files (x86)\Kvaser\Canlib\Inc" ^
  /link /LIBPATH:"C:\Program Files (x86)\Kvaser\Canlib\Lib\x64" ^
  canlib32.lib
```

### 6.2 実行

```bash
# コマンドプロンプトで実行
can_sender.exe

# または PowerShell で実行
.\can_sender.exe
```

### 6.3 出力例

```
==== CAN Sender (using static DBC definitions) ====
Initializing CAN...
✓ CANlib initialized
Opening CAN channel...
✓ CAN channel opened (handle=0)
Setting bus parameters...
✓ Bus parameters set
Starting CAN bus...
✓ CAN bus started

✓ CAN Sender initialized successfully!

CAN transmission start (Ctrl+C to exit)

Message: ENGINE_DATA (ID=123)
Signal 1: ENGINE_Torque (scale=0.1 Nm/unit)
Signal 2: ENGINE_RPM (scale=0.25 rpm/unit)
Period: 1000ms

[1] ENGINE_DATA: RPM=800 rpm, Torque=10 Nm | Hex=[64 00 80 0C 00 00 00 00]
[2] ENGINE_DATA: RPM=850 rpm, Torque=11 Nm | Hex=[6E 00 48 0D 00 00 00 00]
[3] ENGINE_DATA: RPM=900 rpm, Torque=12 Nm | Hex=[78 00 10 0E 00 00 00 00]
[4] ENGINE_DATA: RPM=950 rpm, Torque=13 Nm | Hex=[82 00 D8 0E 00 00 00 00]
[5] ENGINE_DATA: RPM=1000 rpm, Torque=14 Nm | Hex=[8C 00 A0 0F 00 00 00 00]
```

---

## 7. エラーハンドリング

### エラーコード

| エラー | 説明 | 対応 |
|--------|------|------|
| チャネルオープン失敗 | `h < 0` | プログラム終了 |
| バス設定失敗 | `canSetBusParams` 失敗 | コンソール出力、継続 |
| バス開始失敗 | `canBusOn` 失敗 | コンソール出力、継続 |
| フレーム送信失敗 | `canWrite` エラー | エラーコード出力、継続 |

### 出力フォーマット
- **成功**: `✓ メッセージ`
- **エラー**: `✗ エラー説明 (コード: XX)`
- **警告**: `⚠ 警告メッセージ`

---

## 8. パラメータ設定

### 変更可能なパラメータ

```cpp
// クラス内の static constexpr で定義
static constexpr long CAN_ID = 123;              // メッセージID
static constexpr unsigned int PERIOD_MS = 1000;  // 送信周期（ミリ秒）
static constexpr double TORQUE_SCALE = 0.1;     // トルクスケール
static constexpr double RPM_SCALE = 0.25;       // RPM スケール
```

これらを変更してリコンパイルすることで、動作をカスタマイズ可能。

### バス速度設定

```cpp
// setBusParams() 内
canSetBusParams(h, 500000, ...);  // 500 kbps を設定
```

変更可能な速度:
- 125000 (125 kbps)
- 250000 (250 kbps)
- 500000 (500 kbps) ← デフォルト
- 1000000 (1 Mbps)

---

## 9. テスト データ

### テストシナリオ

プログラムは起動時に以下のサイクルでテストデータを送信:

**RPM サイクル**:
- 初期値: 800 rpm
- 増分: 50 rpm / frame
- 最大値: 4000 rpm
- 最大値到達後: 800 rpm にリセット

**Torque サイクル**:
- 初期値: 10 Nm
- 増分: 1 Nm / frame
- 最大値: 200 Nm
- 最大値到達後: 10 Nm にリセット

---

## 10. 依存関係

### 外部ライブラリ
- **KVASER Canlib**: CAN 通信 API

### 標準ライブラリ
- `<iostream>` - コンソール入出力
- `<cstring>` - メモリ操作
- `<cstdio>` - ファイル/文字列操作
- `<direct.h>` - ディレクトリ操作（Windows）
- `<windows.h>` - Windows API

---

## 11. 既知の制限事項

1. **DBC ファイルのサポート未実装**
   - メッセージ定義と信号定義はソースコードに硬化されている
   - DBC ファイルからの動的読み込みは現在実装されていない

2. **複数メッセージ送信未対応**
   - 現在は ENGINE_DATA メッセージのみを送信

3. **受信機能なし**
   - 送信専用プログラム
   - 受信は別途 can_receiver.cpp を使用

4. **割り込み処理未実装**
   - Ctrl+C でのグレースフルシャットダウンは非対応
   - システム強制終了時のみプログラム停止

---

## 12. トラブルシューティング

### Q: "CAN channel opened (handle=-1)" が表示される
**A**: CAN デバイスが接続されていないか、認識されていません。
- デバイスの接続確認
- KVASER Canlib のインストール確認
- デバイスドライバの再認識

### Q: "canWrite failed with status" エラーが出る
**A**: CAN バスに問題があります。
- CAN バス接続確認
- バス速度設定の確認（両端デバイスで一致しているか）
- ケーブル接続確認

### Q: プログラムが起動直後に終了する
**A**: DLL が見つからない可能性があります。
- `canlib32.dll` が実行ファイルと同じディレクトリにあるか確認
- `C:\Program Files (x86)\Kvaser\Canlib\bin_x64\` からコピー

---

## 13. 関連ドキュメント

- [DVL: Kvaser Canlib API リファレンス](https://www.kvaser.com/developer/)
- `message_db.dbc` - CAN メッセージ定義
- `can_receiver.cpp` - 受信プログラム仕様

---

## 14. 変更履歴

| バージョン | 日付 | 変更内容 |
|----------|------|---------|
| 1.0 | 2026-03-29 | 初版作成 |

---

## 15. サポート情報

- **開発者**: AI Assistant (GitHub Copilot)
- **最終更新**: 2026年3月29日
- **ライセンス**: MIT （プロジェクト定義に従う）

