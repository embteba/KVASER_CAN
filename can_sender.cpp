#include <iostream> // 標準入出力ヘッダ
#include <algorithm> // 汎用アルゴリズム（今回: std::lround のため間接的に）
#include <cmath> // 数学関数（std::lround）
#include <cstring> // C スタイル文字列操作（memset）
#include <cstdio> // C 入出力（printf 等、現状は未使用だが残す）
#include <string> // std::string 型
#include <vector> // std::vector 型
#include <direct.h> // ディレクトリ操作（Windows 固有）
#include <windows.h> // Windows API（Sleep, GetModuleFileNameA など）
#include <canlib.h> // KVASER CANlib API ヘッダ

#include "csv_parser.h" // 独自 CSV パーサのヘッダ

namespace {
// ファイル存在チェック: 指定パスが存在しファイルであるか判定する
bool fileExists(const std::string& path) {
    const DWORD attributes = GetFileAttributesA(path.c_str()); // ファイル属性を取得
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0; // 存在かつディレクトリでない
}

// 実行ファイルのあるディレクトリを取得するユーティリティ
std::string getExecutableDirectory() {
    char buffer[MAX_PATH] = {0}; // バッファ初期化
    const DWORD length = GetModuleFileNameA(0, buffer, MAX_PATH); // 実行ファイルのフルパスを取得
    if (length == 0 || length == MAX_PATH) {
        return "."; // 取得失敗時はカレントディレクトリを返す
    }

    const std::string fullPath(buffer, length); // フルパスを std::string に変換
    const size_t separator = fullPath.find_last_of("\\/"); // 最後のパス区切り位置を検索
    if (separator == std::string::npos) {
        return "."; // 区切りが見つからなければカレントディレクトリ
    }

    return fullPath.substr(0, separator); // ディレクトリ部分を返す
}

// CSV ファイルの解決: 指定パス、実行ファイル相対、ワークスペース相対を順に試す
std::string resolveCsvPath(const std::string& requestedPath) {
    if (fileExists(requestedPath)) {
        return requestedPath; // そのままファイルが存在する場合
    }

    const std::string executableRelativePath = getExecutableDirectory() + "\\" + requestedPath; // 実行ファイルディレクトリ + filename
    if (fileExists(executableRelativePath)) {
        return executableRelativePath; // 実行ファイルディレクトリ内のファイルを優先
    }

    const std::string workspaceRelativePath = std::string("KVASER_CAN_\\") + requestedPath; // ワークスペース相対パス
    if (fileExists(workspaceRelativePath)) {
        return workspaceRelativePath; // ワークスペース内に存在する場合
    }

    return requestedPath; // 見つからなければ元のパスを返す（呼び出し元でエラー扱い）
}

// プログラムの使用方法を表示するユーティリティ
void printUsage(const std::string& programName) {
    std::cout << "Usage: " << programName << " [OPTIONS] [CSV_FILE]" << std::endl; // 基本使用法
    std::cout << "\nSend CAN messages from ENGINE_DATA rows in a CSV file over CAN bus." << std::endl; // 概要
    std::cout << "\nOptions:" << std::endl; // オプション一覧
    std::cout << "  CSV_FILE          Path to CSV file (default: engine_test_profile.csv)" << std::endl; // デフォルトCSV
    std::cout << "  -help, --help     Display this help message" << std::endl; // ヘルプオプション
    std::cout << "  -v, --verbose     Enable verbose output" << std::endl; // 詳細表示オプション
    std::cout << "\nCSV Format:" << std::endl; // CSV フォーマット説明
    std::cout << "  Must contain columns: Time, ENGINE_Torque, ENGINE_RPM" << std::endl; // 必須カラム
    std::cout << "  Time in seconds (non-negative, monotonic increasing)" << std::endl; // Time の要件
    std::cout << "  ENGINE_Torque in Nm (0-6553.5)" << std::endl; // Torque の範囲
    std::cout << "  ENGINE_RPM in rpm (0-16383.75)" << std::endl; // RPM の範囲
    std::cout << "\nExamples:" << std::endl; // 使用例
    std::cout << "  " << programName << "                             # Use default CSV file" << std::endl;
    std::cout << "  " << programName << " test_data.csv              # Use custom CSV file" << std::endl;
    std::cout << "  " << programName << " -v test_data.csv           # Verbose output" << std::endl;
}

// 引数がオプション文字列か判定するユーティリティ（今回未使用だが残す）
bool isOption(const std::string& arg) {
    return (arg == "-help" || arg == "--help" || arg == "-h" ||
            arg == "-v" || arg == "--verbose");
}
} // namespace 終了



// CANSender: CSV サンプルを ENGINE_DATA として送信する最小クラス
class CANSender {
private:
    static constexpr long CAN_ID = 123; // 送信メッセージ ID
    static constexpr unsigned int DATA_LENGTH = 8; // フレーム長（バイト）

    canHandle h; // CAN ハンドル
    int messageCount; // 送信カウンタ
    std::vector<EngineSample> samples; // CSV から読み込んだサンプル配列

    /*
     * encodeSignal
     * ENGINE データの物理量を DBC の raw 整数値へ変換する。
     * - 負値は 0 にクリップ
     * - scale で除算し四捨五入して unsigned short にキャストする
     */
    static unsigned short encodeSignal(double value, double scale) {
        if (value < 0.0) value = 0.0; // 負値は 0 にする
        return static_cast<unsigned short>(std::lround(value / scale)); // scale で割って丸め
    }

    // 次サンプルまでの遅延を ms 単位で返す（最小 1ms）
    DWORD getDelayMs(size_t currentIndex) const {
        if (currentIndex + 1 >= samples.size()) return 0; // 最後のサンプルは遅延なし
        const double delta = samples[currentIndex + 1].timeSeconds - samples[currentIndex].timeSeconds; // 時間差
        const long ms = static_cast<long>(delta * 1000.0); // 秒→ミリ秒
        return ms > 0 ? static_cast<DWORD>(ms) : 1U; // 1ms 最低保証
    }

    // 簡易的に CAN をオープン／設定してバスを開始する
    void openSimple() {
        canInitializeLibrary(); // ライブラリ初期化
        h = canOpenChannel(0, canOPEN_ACCEPT_VIRTUAL); // チャネルオープン
        canSetBusParams(h, 500000, 0, 0, 0, 0, 0); // 500 kbps 設定
        canBusOn(h); // バス有効化
    }

    // 簡易クローズ処理
    void closeSimple() {
        if (h >= 0) {
            canBusOff(h); // バス停止
            canClose(h); // チャネルクローズ
            h = -1; // ハンドル無効化
        }
    }

public:
    // コンストラクタ: サンプルを受け取り CAN を開く
    explicit CANSender(const std::vector<EngineSample>& loadedSamples)
        : h(-1), messageCount(0), samples(loadedSamples) {
        openSimple(); // 簡易初期化
    }

    // デストラクタ: CAN を閉じる
    ~CANSender() {
        closeSimple();
    }

    // メイン送信ループ: サンプルを順にエンコードして送信する
    void run() {
        if (samples.empty()) return; // サンプルが無ければ何もしない
        unsigned char msg[DATA_LENGTH] = {0}; // 送信バッファ初期化
        unsigned int dlc = DATA_LENGTH; // データ長

        for (size_t i = 0; i < samples.size(); ++i) {
            const EngineSample &s = samples[i]; // 現在サンプル参照
            memset(msg, 0, sizeof(msg)); // バッファをゼロで初期化
            unsigned short t = encodeSignal(s.engineTorqueNm, 0.1); // トルクを raw に変換（scale=0.1）
            unsigned short r = encodeSignal(s.engineRpm, 0.25); // RPM を raw に変換（scale=0.25）
            msg[0] = t & 0xFF; msg[1] = (t >> 8) & 0xFF; // トルクをリトルエンディアンで配置
            msg[2] = r & 0xFF; msg[3] = (r >> 8) & 0xFF; // RPM をリトルエンディアンで配置

            canWrite(h, CAN_ID, msg, dlc, canMSG_STD); // フレーム送信（エラーは簡易扱い）

            messageCount++; // 送信カウントを増やす
            std::cout << "[" << messageCount << "] t=" << s.timeSeconds << "s RPM=" << s.engineRpm << " Torque=" << s.engineTorqueNm << std::endl; // 最小ログ出力

            const DWORD delay = getDelayMs(i); // 次までの待ち時間
            if (delay > 0) Sleep(delay); // 待機
        }
    }
};

// エントリポイント: 引数解析、CSV 読込、送信実行を行う
int main(int argc, char* argv[])
{
    try {
        const std::string programName = argc > 0 ? argv[0] : "can_sender.exe"; // プログラム名
        bool verbose = false; // verbose フラグ
        std::string csvFilename = "engine_test_profile.csv"; // デフォルト CSV

        if (argc > 1) {
            for (int i = 1; i < argc; ++i) {
                const std::string arg(argv[i]); // 引数を string 化
                if (arg == "-help" || arg == "--help" || arg == "-h") {
                    printUsage(programName); // ヘルプ表示して終了
                    return 0;
                }
                else if (arg == "-v" || arg == "--verbose") {
                    verbose = true; // verbose 有効化
                }
                else if (arg[0] != '-') {
                    csvFilename = arg; // 位置引数: CSV ファイル名
                }
            }
        }

        const std::string csvPath = resolveCsvPath(csvFilename); // CSV パス解決
        if (!fileExists(csvPath)) {
            std::cerr << "✗ CSV file not found: " << csvPath << std::endl; // ファイル未発見エラー
            std::cerr << "   Tried: " << csvFilename << ", from executable dir, from workspace" << std::endl;
            return 1;
        }

        CsvParser parser; // CSV パーサインスタンス
        std::string errorMessage; // エラーメッセージ受け取り用
        if (!parser.load(csvPath, &errorMessage)) {
            std::cerr << "✗ CSV load failed: " << errorMessage << std::endl; // 読込失敗時の出力
            return 1;
        }

        if (verbose) {
            std::cout << "[VERBOSE] CSV loaded successfully" << std::endl; // 詳細ログ
            std::cout << "[VERBOSE] Samples count: " << parser.getSamples().size() << std::endl; // サンプル数
        }

        std::cout << "==== CAN Sender (ENGINE_DATA via CSV playback) ====" << std::endl; // ヘッダ表示
        std::cout << "CSV file: " << csvPath << std::endl; // 使用 CSV パス表示
        
        CANSender sender(parser.getSamples()); // 送信クラス生成（CSV サンプルを渡す）
        std::cout << "\n✓ CAN Sender initialized successfully!\n" << std::endl; // 初期化成功表示
        sender.run(); // 送信ループ実行
        
        if (verbose) {
            std::cout << "[VERBOSE] CSV playback completed successfully" << std::endl; // 完了ログ
        }
    }
    catch (const std::exception& e) {
        std::cerr << "✗ Exception: " << e.what() << std::endl; // 例外発生時の出力
        return 1;
    }
    catch (...) {
        std::cerr << "✗ Unknown exception occurred" << std::endl; // 不明例外
        return 1;
    }

    return 0; // 正常終了
}

