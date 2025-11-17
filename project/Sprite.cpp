#include "Sprite.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"
#include "WinApp.h"

#include <cassert>

using namespace MatrixMath;

// main.cpp に定義されている関数を宣言だけして使う
ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

void Sprite::Initialize(SpriteCommon* spriteCommon) {
    assert(spriteCommon);
    spriteCommon_ = spriteCommon;

    DirectXCommon* dxCommon = spriteCommon_->GetDxCommon();
    ID3D12Device* device = dxCommon->GetDevice();

    // ① 頂点バッファ（4頂点）
    vertexResource_.Reset();
    vertexResource_.Attach(
        CreateBufferResource(device, sizeof(VertexData) * 4));
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

    // 左下
    vertexData_[0].position = { -0.5f, -0.5f, 0.0f, 1.0f };
    vertexData_[0].texcoord = { 0.0f, 1.0f };
    vertexData_[0].normal = { 0.0f, 0.0f,-1.0f };
    // 左上
    vertexData_[1].position = { -0.5f,  0.5f, 0.0f, 1.0f };
    vertexData_[1].texcoord = { 0.0f, 0.0f };
    vertexData_[1].normal = { 0.0f, 0.0f,-1.0f };
    // 右下
    vertexData_[2].position = { 0.5f, -0.5f, 0.0f, 1.0f };
    vertexData_[2].texcoord = { 1.0f, 1.0f };
    vertexData_[2].normal = { 0.0f, 0.0f,-1.0f };
    // 右上
    vertexData_[3].position = { 0.5f,  0.5f, 0.0f, 1.0f };
    vertexData_[3].texcoord = { 1.0f, 0.0f };
    vertexData_[3].normal = { 0.0f, 0.0f,-1.0f };

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * 4);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    // ② インデックスバッファ（2三角形）
    indexResource_.Reset();
    indexResource_.Attach(
        CreateBufferResource(device, sizeof(uint32_t) * 6));
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
    indexData_[0] = 0; indexData_[1] = 1; indexData_[2] = 2;
    indexData_[3] = 2; indexData_[4] = 1; indexData_[5] = 3;

    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    // ③ マテリアル CBuffer
    materialResource_.Reset();
    materialResource_.Attach(
        CreateBufferResource(device, sizeof(Material)));
    materialResource_->Map(0, nullptr,
        reinterpret_cast<void**>(&materialData_));
    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->enableLighting = 0; // ライティング無し
    materialData_->uvTransform = MakeIdentity4x4();

    // ④ 座標変換行列 CBuffer
    transformationMatrixResource_.Reset();
    transformationMatrixResource_.Attach(
        CreateBufferResource(device, sizeof(TransformationMatrix)));
    transformationMatrixResource_->Map(
        0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));

    transformationMatrixData_->WVP = MakeIdentity4x4();
    transformationMatrixData_->World = MakeIdentity4x4();

    // スプライトの初期 Transform（とりあえず画面中央に 100x100）
    transform_.scale = { 100.0f, 100.0f, 1.0f };
    transform_.rotate = { 0.0f, 0.0f, 0.0f };
    transform_.translate = {
        WinApp::kClientWidth * 0.5f,
        WinApp::kClientHeight * 0.5f,
        0.0f
    };
}

void Sprite::Update() {
    if (!transformationMatrixData_) return;

    // World 行列
    Matrix4x4 world = MakeAffine(
        transform_.scale, transform_.rotate, transform_.translate);

    // View 行列：2Dスプライトなので単位行列
    Matrix4x4 view = MakeIdentity4x4();

    // Projection 行列：画面サイズに合わせた正射影
    const float w = static_cast<float>(WinApp::kClientWidth);
    const float h = static_cast<float>(WinApp::kClientHeight);

    Matrix4x4 proj = Orthographic(
        0.0f, 0.0f, w, h, 0.0f, 1.0f);

    Matrix4x4 vp = Multipty(view, proj);
    Matrix4x4 wvp = Multipty(world, vp);

    transformationMatrixData_->WVP = wvp;
    transformationMatrixData_->World = world;
}

void Sprite::Draw() {
    assert(spriteCommon_);
    DirectXCommon* dxCommon = spriteCommon_->GetDxCommon();
    ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

    // スプライト用の共通設定（RootSignature / PSO / トポロジ）
    spriteCommon_->CommonDrawSetting();

    // VB / IB セット
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetIndexBuffer(&indexBufferView_);

    // マテリアル CBuffer (b0)
    commandList->SetGraphicsRootConstantBufferView(
        0, materialResource_->GetGPUVirtualAddress());

    // Transform CBuffer (b1前提)
    commandList->SetGraphicsRootConstantBufferView(
        1, transformationMatrixResource_->GetGPUVirtualAddress());

    // テクスチャ SRV (t0)
    if (textureSrvHandleGPU_.ptr != 0) {
        commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU_);
    }

    // 描画
    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}
