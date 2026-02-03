#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <wrl.h>
#include <d3d12.h>
#include "externals/DirectXTex/DirectXTex.h"
#include "DirectXCommon.h"
#include "SrvManager.h" // 追加
#include "StringUtility.h"

/// テクスチャマネージャ
class TextureManager {
public:
    static TextureManager* GetInstance();

    // SrvManager も受け取る
    void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);
    void Finalize();

    void LoadTexture(const std::string& filePath);
    uint32_t GetTextureIndexByFilePath(const std::string& filePath);
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);
    void ReleaseIntermediateResources();

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
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU{};
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU{};
        uint32_t srvIndex = 0; // 割り当てられたSRVインデックス
    };

    std::vector<TextureData> textureDatas;

    static TextureManager* instance;
    DirectXCommon* dxCommon_ = nullptr;
    SrvManager* srvManager_ = nullptr; // 追加
};