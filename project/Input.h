#pragma once
// ヘッダ側：HINSTANCE と HWND の型情報が必要になるので windows.h をインクルード
#include <windows.h>

// 入力クラス（関数名は英語）
class Input
{
public:
    // 初期化（インスタンスハンドルとウィンドウハンドルを受け取る）
    void Initialize(HINSTANCE hInstance, HWND hwnd);

    // 毎フレーム更新（この回では中身はまだ無し）
    void Update();
};
