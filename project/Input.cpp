#include "Input.h"
// assert マクロを使うために <cassert> をインクルード
#include <cassert>
// ComPtr を使うために <wrl.h> をインクルードし、名前空間を解決
#include <wrl.h>
using namespace Microsoft::WRL;

// DirectInput を使うためのバージョン指定とヘッダ
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

// ライブラリのリンク指定（.lib をプロジェクトにリンク）
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

void Input::Initialize(HINSTANCE hInstance, HWND hwnd)
{
    // ここでは「キーボード入力の初期化部分」をクラスに移植する
    HRESULT result;

    // DirectInput のインスタンス生成
    ComPtr<IDirectInput8> directInput = nullptr;
    result = DirectInput8Create(
        hInstance,                   // アプリのインスタンス
        DIRECTINPUT_VERSION,         // DirectInput のバージョン
        IID_IDirectInput8,           // 生成したいインターフェイスID
        reinterpret_cast<void**>(directInput.GetAddressOf()),
        nullptr
    );
    assert(SUCCEEDED(result));       // 失敗したらここで気付けるようにする

    // キーボードデバイス生成
    ComPtr<IDirectInputDevice8> keyboard = nullptr;
    result = directInput->CreateDevice(
        GUID_SysKeyboard,            // システムキーボード
        keyboard.GetAddressOf(),
        nullptr
    );
    assert(SUCCEEDED(result));

    // 入力データ形式のセット（キーボード用の既定フォーマット）
    result = keyboard->SetDataFormat(&c_dfDIKeyboard);
    assert(SUCCEEDED(result));

    // 排他制御レベルのセット（前面・非独占・Windowsキー無効）
    result = keyboard->SetCooperativeLevel(
        hwnd,
        DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY
    );
    assert(SUCCEEDED(result));

    // ※この回では確認用としてローカルで完結。
    // 　以後の回でメンバ変数化し、Update での読み取りに使う想定。
}

void Input::Update()
{
    // 入力の毎フレーム処理はこの回では未実装
    // 次の回でキーステートの取得等を実装する
}
