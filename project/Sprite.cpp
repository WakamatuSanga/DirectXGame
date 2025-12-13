#include "Sprite.h"
#include "SpriteCommon.h"
#include "Matrix4x4.h"
#include "TextureManager.h"
#include <cassert>

using namespace MatrixMath;

void Sprite::Initialize(SpriteCommon* spriteCommon) {
    assert(spriteCommon);
    spriteCommon_ = spriteCommon;

    // 位置やサイズ、回転の初期値
    position_ = { 0.0f, 0.0f };
    size_ = { 640.0f, 360.0f };
    rotation_ = 0.0f;
    textureIndex_ = 0;
    // 頂点バッファや CBV の生成処理がすでにあるなら、
    // そのコードは残したままで OK（この下に続けてください）。
}

// テクスチャをファイルパスで指定（内部で Load + index 取得）
void Sprite::SetTexture(const std::string& filePath)
{
    TextureManager* texMan = TextureManager::GetInstance();
    texMan->LoadTexture(filePath);  // 未ロードならロードされる
    textureIndex_ = texMan->GetTextureIndexByFilePath(filePath);
}

void Sprite::Update() {
    // 座標・回転・サイズ → Transform に反映
    transform_.translate = { position_.x, position_.y, 0.0f };
    transform_.rotate = { 0.0f, 0.0f, rotation_ };
    transform_.scale = { size_.x, size_.y, 1.0f };

    Matrix4x4 world = MakeAffine(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 vp = MakeIdentity4x4();
    Matrix4x4 wvp = Multipty(world, vp);

    if (transformationMatrixData_) {
        transformationMatrixData_->World = world;
        transformationMatrixData_->WVP = wvp;
    }

    if (vertexData_) {
        vertexData_[0].position = { 0.0f, 1.0f, 0.0f, 1.0f }; // 左下
        vertexData_[1].position = { 0.0f, 0.0f, 0.0f, 1.0f }; // 左上
        vertexData_[2].position = { 1.0f, 1.0f, 0.0f, 1.0f }; // 右下
        vertexData_[3].position = { 1.0f, 0.0f, 0.0f, 1.0f }; // 右上

        // UV（必要ならここで設定）
        vertexData_[0].texcoord = { 0.0f, 1.0f };
        vertexData_[1].texcoord = { 0.0f, 0.0f };
        vertexData_[2].texcoord = { 1.0f, 1.0f };
        vertexData_[3].texcoord = { 1.0f, 0.0f };
    }
}

void Sprite::Draw() {
    assert(spriteCommon_);
    DirectXCommon* dxCommon = spriteCommon_->GetDxCommon();
    assert(dxCommon);

    ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

    // 頂点/インデックスバッファ
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetIndexBuffer(&indexBufferView_);

    // CBV セット（RootParam 0,1）
    if (transformationMatrixResource_) {
        commandList->SetGraphicsRootConstantBufferView(
            0, transformationMatrixResource_->GetGPUVirtualAddress());
    }
    if (materialResource_) {
        commandList->SetGraphicsRootConstantBufferView(
            1, materialResource_->GetGPUVirtualAddress());
    }

    // テクスチャ SRV セット（RootParam 2）
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandle =
        TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex_);
    commandList->SetGraphicsRootDescriptorTable(2, srvHandle);

    // 描画
    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}
