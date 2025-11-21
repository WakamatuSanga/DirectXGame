#include "TextureManager.h"
#include <cassert>

TextureManager* TextureManager::instance = nullptr;
// ImGui が SRV 0 を使うので、テクスチャは 1 番から
uint32_t TextureManager::kSRVIndexTop = 4;

//-----------------------------
// シングルトン
//-----------------------------
TextureManager* TextureManager::GetInstance() {
    if (instance == nullptr) {
        instance = new TextureManager();
    }
    return instance;
}

//-----------------------------
// 初期化／終了
//-----------------------------
void TextureManager::Initialize(DirectXCommon* dxCommon) {
    assert(dxCommon);
    dxCommon_ = dxCommon;

    textureDatas.clear();
    textureDatas.reserve(DirectXCommon::kMaxSRVCount);
}

void TextureManager::Finalize() {
    textureDatas.clear();
    dxCommon_ = nullptr;

    delete instance;
    instance = nullptr;
}

//-----------------------------
// テクスチャ読み込み
//-----------------------------
void TextureManager::LoadTexture(const std::string& filePath) {
    assert(dxCommon_);

    // 読み込み済みテクスチャを検索
    auto it = std::find_if(
        textureDatas.begin(), textureDatas.end(),
        [&](TextureData& data) { return data.filePath == filePath; });
    if (it != textureDatas.end()) {
        // 既に読み込み済みなら何もしない
        return;
    }

    // 最大枚数を超えないようにする
    assert(textureDatas.size() + kSRVIndexTop < DirectXCommon::kMaxSRVCount);

    // テクスチャデータを追加
    textureDatas.resize(textureDatas.size() + 1);
    TextureData& textureData = textureDatas.back();
    textureData.filePath = filePath;   // ★検索用に必ずセット

    ID3D12Device* device = dxCommon_->GetDevice();

    // --- 画像ファイル読み込み ---
    DirectX::ScratchImage image{};
    std::wstring wFilePath = StringUtility::ConvertString(filePath);
    HRESULT hr = DirectX::LoadFromWICFile(
        wFilePath.c_str(),
        DirectX::WIC_FLAGS_FORCE_SRGB,
        &textureData.metadata,
        image);
    assert(SUCCEEDED(hr));

    // MipMap の作成
    DirectX::ScratchImage mipImages{};
    hr = DirectX::GenerateMipMaps(
        image.GetImages(),
        image.GetImageCount(),
        image.GetMetadata(),
        DirectX::TEX_FILTER_SRGB,
        0,
        mipImages);
    assert(SUCCEEDED(hr));

    textureData.metadata = mipImages.GetMetadata();

    // テクスチャリソース生成
    textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);

    // デスクリプタハンドルの計算
    uint32_t srvIndex =
        static_cast<uint32_t>(textureDatas.size() - 1) + kSRVIndexTop;
    textureData.srvHandleCPU = dxCommon_->GetSRVCPUDescriptorHandle(srvIndex);
    textureData.srvHandleGPU = dxCommon_->GetSRVGPUDescriptorHandle(srvIndex);

    // SRV の生成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = textureData.metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = static_cast<UINT>(textureData.metadata.mipLevels);

    device->CreateShaderResourceView(
        textureData.resource.Get(),
        &srvDesc,
        textureData.srvHandleCPU);

    // テクスチャデータ転送（中間リソースを保持しておく：パターン2）
    textureData.intermediateResource =
        dxCommon_->UploadTextureData(textureData.resource, mipImages);
}

//-----------------------------
// 中間リソースをまとめて解放（パターン2）
//-----------------------------
void TextureManager::ReleaseIntermediateResources() {
    for (auto& data : textureDatas) {
        data.intermediateResource.Reset();
    }
}

//-----------------------------
// ファイルパスからテクスチャ番号取得
//-----------------------------
uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath) {
    auto it = std::find_if(
        textureDatas.begin(), textureDatas.end(),
        [&](TextureData& data) { return data.filePath == filePath; });

    if (it != textureDatas.end()) {
        uint32_t textureIndex =
            static_cast<uint32_t>(std::distance(textureDatas.begin(), it));
        return textureIndex;
    }

    // 読み込み忘れ
    assert(false);
    return 0;
}

//-----------------------------
// テクスチャ番号から GPU ハンドル取得
//-----------------------------
D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(uint32_t textureIndex) {
    assert(textureIndex < textureDatas.size());
    TextureData& textureData = textureDatas[textureIndex];
    return textureData.srvHandleGPU;
}
