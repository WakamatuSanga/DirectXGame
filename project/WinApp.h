#pragma once

#include <Windows.h>
#include <cstdint>

class WinApp {
public:
    // クライアント領域のサイズ（スライドの定数）
    static const int32_t kClientWidth = 1280;
    static const int32_t kClientHeight = 720;

public:
    WinApp() = default;
    ~WinApp();             // 終了時にウィンドウを閉じる

    // --- 静的メンバ関数（ウィンドウプロシージャ）---
    static LRESULT CALLBACK WindowProc(HWND hwnd,
        UINT msg,
        WPARAM wparam,
        LPARAM lparam);

    // --- メンバ関数 ---
    void Initialize();     // WindowsAPI の初期化

    // --- getter ---
    HWND      GetHwnd()      const { return hwnd; }
    HINSTANCE GetHInstance() const { return wc.hInstance; }

private:
    // --- メンバ変数 ---
    WNDCLASS wc{};          // ウィンドウクラス
    HWND     hwnd = nullptr;
};
