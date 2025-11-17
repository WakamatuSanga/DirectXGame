#pragma once

// 前方宣言
class DirectXCommon;

// スプライト共通部
class SpriteCommon {
public:
    // 初期化
    void Initialize(DirectXCommon* dxCommon);

    // DirectXCommon を貸し出す getter
    DirectXCommon* GetDirectXCommon() const { return dxCommon_; }

private:
    // DirectXCommon のインスタンスを保持しておく
    DirectXCommon* dxCommon_ = nullptr;
};
