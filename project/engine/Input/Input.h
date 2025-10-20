#pragma once
#include <Windows.h>

// DirectInput はヘッダを読む“前”にバージョン指定が必要
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <array>
#include <wrl.h>

class Input {
public:
    // クラス内だけで使うエイリアス（using namespace を広げない）
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    // 初期化：アプリ実体と対象ウィンドウを受け取る
    void Initialize(HINSTANCE appInstance, HWND windowHandle);

    // 1フレーム分の更新
    void Update();

    // クエリ
    bool IsPressed(int dik)   const; // 押下中
    bool IsTriggered(int dik) const; // 今フレームで押された
    bool IsReleased(int dik)  const; // 今フレームで離された

private:
    // フォーカス喪失時などに再取得する
    void AcquireIfLost();

    ComPtr<IDirectInput8>       directInput_{};
    ComPtr<IDirectInputDevice8> keyboardDevice_{};
    std::array<BYTE, 256>       currentKeys_{};   // 現在のスキャンコード状態
    std::array<BYTE, 256>       previousKeys_{};  // 1フレーム前
    HWND                        windowHandle_{};  // 協調レベル設定に使う
};
