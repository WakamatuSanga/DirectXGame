// SpriteCommon.h
#pragma once
#include "DirectXCommon.h"

class SpriteCommon {
public:
    // 初期化
    void Initialize(DirectXCommon* dxCommon);

    // getter（スライド「getter」）
    DirectXCommon* GetDxCommon() const { return dxCommon_; }

    // 共通描画設定（スライド「共通描画設定」）
    void CommonDrawSetting();

private:
    // スライド「処理を関数にまとめる」
    // ルートシグネチャの作成
    void CreateRootSignature();
    // グラフィックスパイプラインの生成
    void CreateGraphicsPipelineState();

private:
    // DirectXCommon のポインタ（スライド「DirectXCommonのポインタ」）
    DirectXCommon* dxCommon_ = nullptr;

    // Sprite 用のルートシグネチャ & パイプラインステート
    ID3D12RootSignature* rootSignature_ = nullptr;
    ID3D12PipelineState* pipelineState_ = nullptr;
};
