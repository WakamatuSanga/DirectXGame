#pragma once
#include "DirectXCommon.h"
#include <wrl.h>

class Object3dCommon
{
public:
    // 初期化
    void Initialize(DirectXCommon* dxCommon);

    // 共通描画設定
    void CommonDrawSetting();

    // ゲッター
    DirectXCommon* GetDxCommon() const { return dxCommon_; }

private:
    // ルートシグネチャの作成
    void CreateRootSignature();
    // グラフィックスパイプラインの生成
    void CreateGraphicsPipelineState();

private:
    DirectXCommon* dxCommon_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;
};