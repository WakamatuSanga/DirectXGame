#include "Input.h"
#include <cassert>
#include <wrl.h>      // ComPtr
using Microsoft::WRL::ComPtr;

// DirectInput
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

// ライブラリリンク（プロパティ設定派なら削ってOK）
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

class InputImplDirect {
public:
    ComPtr<IDirectInput8>        di_;
    ComPtr<IDirectInputDevice8>  keyboard_;
    BYTE now_[256]{};
    BYTE prev_[256]{};
};

void Input::Initialize(HINSTANCE hInstance, HWND hwnd) {
    assert(hInstance);
    assert(hwnd);

    // 簡単実装：Implを使わずに直持ちでも構いません
    auto* impl = new InputImplDirect();

    HRESULT hr = S_OK;

    // DirectInput インスタンス生成
    hr = DirectInput8Create(
        hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
        reinterpret_cast<void**>(impl->di_.GetAddressOf()), nullptr);
    assert(SUCCEEDED(hr));

    // キーボードデバイス生成
    hr = impl->di_->CreateDevice(GUID_SysKeyboard, impl->keyboard_.GetAddressOf(), nullptr);
    assert(SUCCEEDED(hr));

    // 入力データ形式
    hr = impl->keyboard_->SetDataFormat(&c_dfDIKeyboard);
    assert(SUCCEEDED(hr));

    // 協調レベル
    hr = impl->keyboard_->SetCooperativeLevel(
        hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
    assert(SUCCEEDED(hr));

    // 最初にAcquireしておく（Updateで再取得もケア）
    impl->keyboard_->Acquire();

    p_ = reinterpret_cast<Input::Impl*>(impl);
}

void Input::Update() {
    auto* impl = reinterpret_cast<InputImplDirect*>(p_);
    assert(impl);

    // 前フレームを保存
    memcpy(impl->prev_, impl->now_, sizeof(impl->now_));

    // 状態取得（フォーカス外れ時の再取得に対応）
    HRESULT hr = impl->keyboard_->GetDeviceState(sizeof(impl->now_), impl->now_);
    if (FAILED(hr)) {
        // 取り戻すまでAcquireを試みる
        while (hr == DIERR_INPUTLOST) {
            hr = impl->keyboard_->Acquire();
            if (SUCCEEDED(hr)) {
                hr = impl->keyboard_->GetDeviceState(sizeof(impl->now_), impl->now_);
            } else {
                break;
            }
        }
        if (hr == DIERR_NOTACQUIRED) {
            impl->keyboard_->Acquire();
            impl->keyboard_->GetDeviceState(sizeof(impl->now_), impl->now_);
        }
    }
}

void Input::Finalize() {
    auto* impl = reinterpret_cast<InputImplDirect*>(p_);
    if (!impl) return;
    if (impl->keyboard_) { impl->keyboard_->Unacquire(); }
    delete impl;
    p_ = nullptr;
}

Input::~Input() {
    Finalize();
}

// ---------- helpers ----------
bool Input::IsDown(int dik) const {
    auto* impl = reinterpret_cast<InputImplDirect*>(p_);
    return impl && (impl->now_[dik] & 0x80);
}
bool Input::IsPressed(int dik) const {
    auto* impl = reinterpret_cast<InputImplDirect*>(p_);
    return impl && (impl->now_[dik] & 0x80) && !(impl->prev_[dik] & 0x80);
}
bool Input::IsReleased(int dik) const {
    auto* impl = reinterpret_cast<InputImplDirect*>(p_);
    return impl && !(impl->now_[dik] & 0x80) && (impl->prev_[dik] & 0x80);
}
