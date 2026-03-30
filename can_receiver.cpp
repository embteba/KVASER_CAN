#include <iostream>
// Windows API を使用するためのヘッダ
#include <windows.h>
// KVASER Canlib の API ヘッダ
#include <canlib.h>

// 最小限の ENGINE_DATA 受信クラスを提供するファイル
// 以下は各行に対する簡潔な説明コメントを付与した実装

// クラス: CANReceiver
// 目的: CAN バスから ENGINE_DATA (ID=123) を受信し、デコードして出力する
class CANReceiver {
private:
    // 定数: フレームのデータ長（バイト）
    static constexpr unsigned int DATA_LENGTH = 8;
    // 定数: ENGINE_DATA メッセージの CAN ID（10 進で指定）
    static constexpr long ENGINE_DATA_ID = 123;
    // メンバ: CAN ハンドル
    canHandle h;
    // メンバ: 受信したメッセージ数のカウンタ
    int messageCount;

    // メソッド: canlib を初期化し、チャネルを開いてバスを開始する（簡易版）
    void openSimple() {
        // canlib を初期化する
        canInitializeLibrary();
        // チャネル 0 をオープン（仮想デバイス受け入れフラグを指定）
        h = canOpenChannel(0, canOPEN_ACCEPT_VIRTUAL);
        // バスパラメータ（500 kbps）を設定する
        canSetBusParams(h, 500000, 0, 0, 0, 0, 0);
        // CAN バスを ON にする
        canBusOn(h);
    }

    // メソッド: CAN バスを停止してハンドルを閉じる（簡易版）
    void closeSimple() {
        // ハンドルが有効な場合のみオフ処理をする
        if (h >= 0) {
            // バスを OFF にする
            canBusOff(h);
            // チャネルを閉じる
            canClose(h);
            // ハンドルを無効値にセット
            h = -1;
        }
    }

    // メソッド: 受信したフレームのデータを DBC として解釈して物理量に変換する
    void decodeEngineData(const unsigned char *msg, unsigned int dlc, double &torqueNm, double &rpm) {
        // DLC が 4 バイト未満の場合はデコードできないため 0 を返す
        if (dlc < 4) { torqueNm = 0.0; rpm = 0.0; return; }
        // bytes 0-1 をリトルエンディアンの unsigned short として結合してトルクの raw 値とする
        unsigned short t = (unsigned short)msg[0] | ((unsigned short)msg[1] << 8);
        // bytes 2-3 をリトルエンディアンの unsigned short として結合して RPM の raw 値とする
        unsigned short r = (unsigned short)msg[2] | ((unsigned short)msg[3] << 8);
        // スケールをかけて物理量に変換（トルク: scale=0.1、RPM: scale=0.25）
        torqueNm = static_cast<double>(t) * 0.1;
        rpm = static_cast<double>(r) * 0.25;
    }

public:
    // コンストラクタ: メンバ初期化と CAN の簡易オープンを行う
    CANReceiver() : h(-1), messageCount(0) { openSimple(); }
    // デストラクタ: CAN の簡易クローズを行う
    ~CANReceiver() { closeSimple(); }

    // メソッド: メイン受信ループ（無限ループ）
    void run() {
        // 受信パラメータを初期化
        long id = 0; // 受信メッセージの ID を格納する変数
        unsigned char msg[DATA_LENGTH]; // 受信データバッファ
        unsigned int dlc = 0; // 受信フレームの DLC
        unsigned int flags = 0; // 受信フラグ（未使用だが取得する）
        unsigned long timestamp = 0; // 受信タイムスタンプ（ミリ秒）

        // 動作開始を示すメッセージを出力
        std::cout << "CAN receiver running (minimal)" << std::endl;
        // 無限ループで canRead をポーリングする
        while (true) {
            // canRead を呼んでキューからメッセージを取得する
            canStatus s = canRead(h, &id, msg, &dlc, &flags, &timestamp);
            // canRead が成功したら処理を継続する
            if (s == canOK) {
                // 受信 ID が ENGINE_DATA の場合のみデコードして出力する
                if (id == ENGINE_DATA_ID) {
                    double torque = 0.0, rpm = 0.0; // 物理量格納用
                    // 受信データをデコードして物理量へ変換
                    decodeEngineData(msg, dlc, torque, rpm);
                    // デコード結果をコンソールへ出力
                    std::cout << "ENGINE_DATA: Torque=" << torque << " Nm, RPM=" << rpm << std::endl;
                }
            }
            // 次のポーリングまで少し待機して CPU 負荷を下げる
            Sleep(50);
        }
    }
};

// エントリポイント: CANReceiver を生成して受信ループを起動する
int main() {
    // CANReceiver インスタンスを作成
    CANReceiver r;
    // 受信ループを実行（この呼び出しは戻ってこない）
    r.run();
    // 通常到達しないが安全のため 0 を返す
    return 0;
}
