#include "WinApp.h"

#include <cassert>

// ImGui の Win32 用ハンドラ
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// --------------------
// ウィンドウプロシージャ
// --------------------
LRESULT CALLBACK WinApp::WindowProc(HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam) {

    // ImGui に先に処理させる
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
        return true;
    }

    // メッセージごとの処理
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // 既定の処理
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

// --------------------
// 初期化
// --------------------
void WinApp::Initialize() {

    // ウィンドウクラス登録
    wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = L"CG2WindowClass";
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);

    // クライアント領域サイズ
    RECT wrc{ 0, 0, kClientWidth, kClientHeight };

    // 実ウィンドウサイズへ変換
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, FALSE);

    // ウィンドウ生成
    hwnd = CreateWindow(
        wc.lpszClassName,         // クラス名
        L"CG2",                   // タイトル
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wrc.right - wrc.left,
        wrc.bottom - wrc.top,
        nullptr, nullptr,
        wc.hInstance,
        nullptr);

    assert(hwnd != nullptr);

    // 表示
    ShowWindow(hwnd, SW_SHOW);
}

// --------------------
// 終了処理
// --------------------
WinApp::~WinApp() {
    if (hwnd) {
        CloseWindow(hwnd);    // もともと main.cpp の最後にあったやつ
        hwnd = nullptr;
    }
}
