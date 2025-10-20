#include "Input.h"
#include <cstring> // std::memset

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

void Input::Initialize(HINSTANCE appInstance, HWND windowHandle) {
    windowHandle_ = windowHandle;

    // DirectInput 本体
    if (FAILED(DirectInput8Create(appInstance, DIRECTINPUT_VERSION,
        IID_IDirectInput8, (void**)&directInput_, nullptr))) {
        throw std::runtime_error("DirectInput8Create failed");
    }

    // キーボードデバイス
    if (FAILED(directInput_->CreateDevice(GUID_SysKeyboard, &keyboardDevice_, nullptr))) {
        throw std::runtime_error("CreateDevice(GUID_SysKeyboard) failed");
    }

    // データ形式（必須）
    if (FAILED(keyboardDevice_->SetDataFormat(&c_dfDIKeyboard))) {
        throw std::runtime_error("SetDataFormat(c_dfDIKeyboard) failed");
    }

    // 協調レベル（一般的な設定）
    if (FAILED(keyboardDevice_->SetCooperativeLevel(windowHandle_,
        DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY))) {
        throw std::runtime_error("SetCooperativeLevel failed");
    }

    // 最初に取得
    keyboardDevice_->Acquire();
}

void Input::AcquireIfLost() {
    // まず読み取りを試す
    HRESULT hr = keyboardDevice_->GetDeviceState(
        static_cast<DWORD>(currentKeys_.size()), currentKeys_.data());

    if (FAILED(hr)) {
        // フォーカス喪失時などは Acquire を繰り返すことがある
        hr = keyboardDevice_->Acquire();
        while (hr == DIERR_INPUTLOST) {
            hr = keyboardDevice_->Acquire();
        }
        if (SUCCEEDED(hr)) {
            hr = keyboardDevice_->GetDeviceState(
                static_cast<DWORD>(currentKeys_.size()), currentKeys_.data());
        }
    }

    // それでも失敗するケースは安全側に倒す
    if (FAILED(hr)) {
        std::memset(currentKeys_.data(), 0, currentKeys_.size());
    }
}

void Input::Update() {
    // 1フレーム前を保存
    previousKeys_ = currentKeys_;
    // 取得（必要なら再取得を含む）
    AcquireIfLost();
}

bool Input::IsPressed(int dik) const {
    return (currentKeys_[dik] & 0x80) != 0;
}
bool Input::IsTriggered(int dik) const {
    return (currentKeys_[dik] & 0x80) && !(previousKeys_[dik] & 0x80);
}
bool Input::IsReleased(int dik) const {
    return !(currentKeys_[dik] & 0x80) && (previousKeys_[dik] & 0x80);
}
