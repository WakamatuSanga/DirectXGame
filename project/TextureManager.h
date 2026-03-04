#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <wrl.h>
#include <d3d12.h>
#include <memory>
#include "externals/DirectXTex/DirectXTex.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "StringUtility.h"

/// テクスチャマネージャ
class TextureManager {
    
    friend struct std::default_delete<TextureManager>;

public:
    static TextureManager* GetInstance();

    void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);
    void Finalize();

    void LoadTexture(const std::string& filePath);
    uint32_t GetTextureIndexByFilePath(const std::string& filePath);
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);
    void ReleaseIntermediateResources();
    const DirectX::TexMetadata& GetMetaData(uint32_t textureIndex);

private:
    TextureManager() = default;
    ~TextureManager() = default;
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    struct TextureData {
        std::string filePath;
        DirectX::TexMetadata metadata;
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
        uint32_t srvIndex;
    };

    static std::unique_ptr<TextureManager> instance_;

    std::vector<TextureData> textureDatas;

    DirectXCommon* dxCommon_ = nullptr;
    SrvManager* srvManager_ = nullptr;
};