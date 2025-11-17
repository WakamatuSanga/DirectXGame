#include "SpriteCommon.h"
#include <d3d12.h>
#include <cassert>

void SpriteCommon::Initialize(DirectXCommon* dxCommon)
{
    // DirectXCommon のポインタを覚えておく（絶対に new / delete しない）
    dxCommon_ = dxCommon;

    // スライドの説明通り、「グラフィックスパイプラインの生成」の中から
    // 最初にルートシグネチャの作成を呼ぶ設計にする
    CreateGraphicsPipelineState();
}

// ルートシグネチャの作成 ------------------------------------------
void SpriteCommon::CreateRootSignature()
{
    assert(dxCommon_);
    ID3D12Device* device = dxCommon_->GetDevice();

    // ===== Sprite/Particle 用 RootSignature =====
    // main.cpp に今書いてある
    //   D3D12_ROOT_PARAMETER sprParams[...]{};
    //   D3D12_STATIC_SAMPLER_DESC staticSamplers[...]{};
    //   D3D12_ROOT_SIGNATURE_DESC sprRootDesc{};
    //   D3D12SerializeRootSignature(...)
    //   device->CreateRootSignature(...)
    // のブロックを「spriteRootSignature」ではなく
    // この SpriteCommon::rootSignature_ を使う形にして
    // まるごとここへ移動してください。

    // 例）ほんのイメージ
    /*
    D3D12_ROOT_PARAMETER sprParams[3]{};
    // ... ここは main.cpp の設定をコピー ...

    D3D12_ROOT_SIGNATURE_DESC sprRootDesc{};
    sprRootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    sprRootDesc.pParameters = sprParams;
    sprRootDesc.NumParameters = _countof(sprParams);
    sprRootDesc.pStaticSamplers = staticSamplers;
    sprRootDesc.NumStaticSamplers = _countof(staticSamplers);

    ID3DBlob* sprSigBlob = nullptr;
    ID3DBlob* sprErr = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&sprRootDesc,
        D3D_ROOT_SIGNATURE_VERSION_1, &sprSigBlob, &sprErr);
    assert(SUCCEEDED(hr));

    hr = device->CreateRootSignature(
        0,
        sprSigBlob->GetBufferPointer(),
        sprSigBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));

    sprSigBlob->Release();
    if (sprErr) { sprErr->Release(); }
    */
}

// グラフィックスパイプラインの生成 -------------------------------
void SpriteCommon::CreateGraphicsPipelineState()
{
    assert(dxCommon_);
    ID3D12Device* device = dxCommon_->GetDevice();

    // まずルートシグネチャを作る（スライドの黄色マーカーの内容）
    CreateRootSignature();

    // ===== Sprite/Particle 用 PSO =====
    // main.cpp に今書いてある
    //   D3D12_GRAPHICS_PIPELINE_STATE_DESC sprPsoDesc = graphicsPipelineStateDesc;
    //   sprPsoDesc.pRootSignature = spriteRootSignature;
    //   sprPsoDesc.VS = { particleVS->GetBufferPointer(), ... };
    //   sprPsoDesc.PS = { particlePS->GetBufferPointer(), ... };
    //   device->CreateGraphicsPipelineState(..., &psoSpriteParticles);
    // のブロックを、「psoSpriteParticles」ではなく
    // この SpriteCommon::pipelineState_ に作るようにして
    // 丸ごとここへ移動してください。

    // 例イメージ：
    /*
    D3D12_GRAPHICS_PIPELINE_STATE_DESC sprPsoDesc = {};
    // 基本は 3D の graphicsPipelineStateDesc をベースにしてもOK
    // sprPsoDesc = graphicsPipelineStateDesc; みたいにコピーしてから
    // RootSignature だけ Sprite 用の rootSignature_ に差し替えるイメージ。

    sprPsoDesc.pRootSignature = rootSignature_;
    sprPsoDesc.VS = { particleVS->GetBufferPointer(), particleVS->GetBufferSize() };
    sprPsoDesc.PS = { particlePS->GetBufferPointer(), particlePS->GetBufferSize() };
    // DepthWrite を OFF にする等、main.cpp に書いてある設定も全部コピー

    HRESULT hr = device->CreateGraphicsPipelineState(
        &sprPsoDesc, IID_PPV_ARGS(&pipelineState_));
    assert(SUCCEEDED(hr));
    */
}

// 共通描画設定 -----------------------------------------------------
void SpriteCommon::CommonDrawSetting()
{
    assert(dxCommon_);
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    // ルートシグネチャをセットするコマンド
    commandList->SetGraphicsRootSignature(rootSignature_);

    // グラフィックスパイプラインステートをセットするコマンド
    commandList->SetPipelineState(pipelineState_);

    // プリミティブトポロジーをセットするコマンド
    // （スプライトは三角形２枚なので TRIANGLELIST）
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
