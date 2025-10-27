#include "Input.h"
#include <cassert>   // assert マクロを使うため

// ライブラリの自動リンク（.lib をプロジェクトに追加しなくてもリンクされる）
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

void Input::Initialize(HINSTANCE hInstance, HWND hwnd)
{
    // DirectInput のインスタンス生成（ローカルで十分）
    HRESULT result = S_OK;                                    // ここでエラー確認に使う
    ComPtr<IDirectInput8> di = nullptr;                       // DirectInput8 の本体
    result = DirectInput8Create(
        hInstance,
        DIRECTINPUT_VERSION,
        IID_IDirectInput8,
        reinterpret_cast<void**>(di.GetAddressOf()),
        nullptr
    );
    assert(SUCCEEDED(result));  // 失敗していないかを即時検出

    // キーボードデバイスの生成
    result = di->CreateDevice(GUID_SysKeyboard, &keyboard, nullptr);
    assert(SUCCEEDED(result));

    // 入力データ形式のセット（キーボード既定フォーマット）
    result = keyboard->SetDataFormat(&c_dfDIKeyboard);
    assert(SUCCEEDED(result));

    // 協調レベルの設定（前面・非排他・Windowsキー無効）
    result = keyboard->SetCooperativeLevel(
        hwnd,
        DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY
    );
    assert(SUCCEEDED(result));
}

void Input::Update()
{
    // キーボード情報の取得開始
    keyboard->Acquire();

    // 全キーの入力情報を取得（256 バイトの配列に状態が入る）
    keyboard->GetDeviceState(sizeof(key_), key_);
}

bool Input::IsKeyDown(int dik) const
{
    // DirectInput のキー状態は最上位ビットが 1 なら押下
    return (key_[dik] & 0x80) != 0;
}
