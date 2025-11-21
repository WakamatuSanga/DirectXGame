#include "SpriteCommon.h"
#include <cassert>
#include "Matrix4x4.h" // Transform 用（必要なら）
using namespace MatrixMath;
using Microsoft::WRL::ComPtr;

void SpriteCommon::Initialize(DirectXCommon* dxCommon)
{
    dxCommon_ = dxCommon;
    CreateGraphicsPipelineState();
}

// ルートシグネチャの作成 ------------------------------------------
void SpriteCommon::CreateRootSignature()
{
    assert(dxCommon_);
    ID3D12Device* device = dxCommon_->GetDevice();

    // ==== Static Sampler ====
    D3D12_STATIC_SAMPLER_DESC staticSamplers[1]{};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // ==== Root Parameters ====
    D3D12_ROOT_PARAMETER sprParams[3]{};

    // [0] Pixel CBV : Material(b0)
    sprParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    sprParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    sprParams[0].Descriptor.ShaderRegister = 0;

    // [1] Vertex SRV(Instancing用 StructuredBuffer) : t0
    D3D12_DESCRIPTOR_RANGE instRange{};
    instRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    instRange.BaseShaderRegister = 0;
    instRange.NumDescriptors = 1;
    instRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    sprParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    sprParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    sprParams[1].DescriptorTable.pDescriptorRanges = &instRange;
    sprParams[1].DescriptorTable.NumDescriptorRanges = 1;

    // [2] Pixel SRV(Texture) : t0
    D3D12_DESCRIPTOR_RANGE texRange{};
    texRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    texRange.BaseShaderRegister = 0;
    texRange.NumDescriptors = 1;
    texRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    sprParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    sprParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    sprParams[2].DescriptorTable.pDescriptorRanges = &texRange;
    sprParams[2].DescriptorTable.NumDescriptorRanges = 1;

    // ==== RootSignatureDesc ====
    D3D12_ROOT_SIGNATURE_DESC sprRootDesc{};
    sprRootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    sprRootDesc.pParameters = sprParams;
    sprRootDesc.NumParameters = _countof(sprParams);
    sprRootDesc.pStaticSamplers = staticSamplers;
    sprRootDesc.NumStaticSamplers = _countof(staticSamplers);

    ComPtr<ID3DBlob> sigBlob;
    ComPtr<ID3DBlob> errBlob;
    HRESULT hr = D3D12SerializeRootSignature(
        &sprRootDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        sigBlob.GetAddressOf(),
        errBlob.GetAddressOf()
    );

    if (FAILED(hr)) {
        if (errBlob) {
            OutputDebugStringA(
                (char*)errBlob->GetBufferPointer()
            );
        }
        assert(false);
    }

    hr = device->CreateRootSignature(
        0,
        sigBlob->GetBufferPointer(),
        sigBlob->GetBufferSize(),
        IID_PPV_ARGS(rootSignature_.GetAddressOf())
    );
    assert(SUCCEEDED(hr));
}
// グラフィックスパイプラインの生成 -------------------------------
void SpriteCommon::CreateGraphicsPipelineState()
{
    assert(dxCommon_);
    ID3D12Device* device = dxCommon_->GetDevice();

    // まずルートシグネチャ
    CreateRootSignature();

    // シェーダー読み込み（パスはプロジェクトに合わせて変更してOK）
    auto vsBlob = dxCommon_->CompileShader(
        L"resources/shaders/Particle.VS.hlsl", L"vs_6_0");
    auto psBlob = dxCommon_->CompileShader(
        L"resources/shaders/Particle.PS.hlsl", L"ps_6_0");

    // 入力レイアウト（Sprite::VertexData に合わせる）
    D3D12_INPUT_ELEMENT_DESC inputElements[3]{};
    inputElements[0] = {
        "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,
        0, D3D12_APPEND_ALIGNED_ELEMENT,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
    };
    inputElements[1] = {
        "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
        0, D3D12_APPEND_ALIGNED_ELEMENT,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
    };
    inputElements[2] = {
        "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,
        0, D3D12_APPEND_ALIGNED_ELEMENT,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
    };

    // ブレンド（アルファブレンド）
    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // ラスタライザ
    D3D12_RASTERIZER_DESC rasterDesc{};
    rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterDesc.DepthClipEnable = TRUE;

    // 深度ステンシル（Sprites は基本 Depth OFF）
    D3D12_DEPTH_STENCIL_DESC depthDesc{};
    depthDesc.DepthEnable = FALSE;
    depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    depthDesc.StencilEnable = FALSE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psoDesc.BlendState = blendDesc;
    psoDesc.RasterizerState = rasterDesc;
    psoDesc.DepthStencilState = depthDesc;
    psoDesc.InputLayout.pInputElementDescs = inputElements;
    psoDesc.InputLayout.NumElements = _countof(inputElements);
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    HRESULT hr = device->CreateGraphicsPipelineState(
        &psoDesc, IID_PPV_ARGS(pipelineState_.GetAddressOf()));
    assert(SUCCEEDED(hr));
}

// 共通描画設定 -----------------------------------------------------
void SpriteCommon::CommonDrawSetting()
{
    assert(dxCommon_);
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    commandList->SetPipelineState(pipelineState_.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
