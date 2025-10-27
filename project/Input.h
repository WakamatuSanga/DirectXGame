#pragma once
// Windows の基本型（HINSTANCE, HWND など）を使うため
#include <windows.h>
// ヘッダ側でも ComPtr を使うため
#include <wrl.h>
// DirectInput のバージョン指定は dinput.h より前に必要
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <cstdint>

// 入力クラス（キーボード）
class Input {
public:
    // 【エイリアステンプレート】ヘッダで using namespace を広げないための短縮形
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
    // 初期化：アプリのインスタンスとウィンドウハンドルを受け取る
    void Initialize(HINSTANCE hInstance, HWND hwnd);  // 日本語名は使わない

    // 更新：毎フレーム呼び出してキーボード状態を取得する
    void Update();

    // 便利関数：指定キーが押されているか（例：IsKeyDown(DIK_SPACE)）
    bool IsKeyDown(int dik) const;

private:
    // キーボードのデバイス（メンバ変数にして再利用）
    ComPtr<IDirectInputDevice8> keyboard{};

    // 直近フレームのキー状態（DirectInput は 256 キー）
    BYTE key_[256] = {};
};
