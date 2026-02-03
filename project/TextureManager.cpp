#include "TextureManager.h"
#include <cassert>

TextureManager* TextureManager::instance = nullptr;

TextureManager* TextureManager::GetInstance() {
    if (instance == nullptr) {
        instance = new TextureManager();
    }
    return instance;
}

void TextureManager::Initialize(DirectXCommon* dxCommon, SrvManager* srvManager) {
    assert(dxCommon);
    assert(srvManager);
    dxCommon_ = dxCommon;
    srvManager_ = srvManager;

    textureDatas.clear();
    textureDatas.reserve(DirectXCommon::kMaxSRVCount);
}

void TextureManager::Finalize() {
    textureDatas.clear();
    dxCommon_ = nullptr;
    srvManager_ = nullptr;

    delete instance;
    instance = nullptr;
}

void TextureManager::LoadTexture(const std::string& filePath) {
    assert(dxCommon_);

    // 読み込み済みチェック
    auto it = std::find_if(
        textureDatas.begin(), textureDatas.end(),
        [&](TextureData& data) { return data.filePath == filePath; });
    if (it != textureDatas.end()) {
        return;
    }

    assert(srvManager_->CanAllocate());

    // テクスチャデータを追加
    textureDatas.resize(textureDatas.size() + 1);
    TextureData& textureData = textureDatas.back();
    textureData.filePath = filePath;

    // 画像読み込み
    DirectX::ScratchImage image{};
    std::wstring wFilePath = StringUtility::ConvertString(filePath);
    HRESULT hr = DirectX::LoadFromWICFile(
        wFilePath.c_str(),
        DirectX::WIC_FLAGS_FORCE_SRGB,
        &textureData.metadata,
        image);
    assert(SUCCEEDED(hr));

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
    textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);
    textureData.intermediateResource = dxCommon_->UploadTextureData(textureData.resource, mipImages);

    // ★ SrvManager からインデックスを確保し、SRV生成
    textureData.srvIndex = srvManager_->Allocate();
    textureData.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(textureData.srvIndex);
    textureData.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(textureData.srvIndex);

    srvManager_->CreateSRVforTexture2D(
        textureData.srvIndex,
        textureData.resource.Get(),
        textureData.metadata.format,
        static_cast<UINT>(textureData.metadata.mipLevels)
    );
}

void TextureManager::ReleaseIntermediateResources() {
    for (auto& data : textureDatas) {
        data.intermediateResource.Reset();
    }
}

uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath) {
    auto it = std::find_if(
        textureDatas.begin(), textureDatas.end(),
        [&](TextureData& data) { return data.filePath == filePath; });

    if (it != textureDatas.end()) {
        uint32_t textureIndex =
            static_cast<uint32_t>(std::distance(textureDatas.begin(), it));
        return textureIndex;
    }
    assert(false);
    return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(uint32_t textureIndex) {
    assert(textureIndex < textureDatas.size());
    TextureData& textureData = textureDatas[textureIndex];
    return textureData.srvHandleGPU;
}