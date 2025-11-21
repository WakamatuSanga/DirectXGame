#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <wrl.h>
#include <d3d12.h>
#include "externals/DirectXTex/DirectXTex.h"
#include "DirectXCommon.h"
#include "StringUtility.h"

/// テクスチャマネージャ
class TextureManager {
public:
    // シングルトンインスタンス取得
    static TextureManager* GetInstance();

    // 初期化／終了
    void Initialize(DirectXCommon* dxCommon); // ★ dxCommon をもらう
    void Finalize();

    /// テクスチャファイルの読み込み  
    /// （同じパスを複数回呼んでも1回しか読み込まない）
    void LoadTexture(const std::string& filePath);

    /// ファイルパスからテクスチャ番号を取得
    uint32_t GetTextureIndexByFilePath(const std::string& filePath);

    /// テクスチャ番号から GPU ハンドル(SRV) を取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);

    /// （パターン2用）中間リソースをまとめて解放
    void ReleaseIntermediateResources();

private:
    // シングルトン
    TextureManager() = default;
    ~TextureManager() = default;
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    // テクスチャ1枚分のデータ
    struct TextureData {
        std::string filePath;                         // ファイルパス
        DirectX::TexMetadata metadata;                // メタデータ(幅・高さなど)
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;             // テクスチャ本体
        Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource; // 転送用一時リソース
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU{};   // SRV CPUハンドル
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU{};   // SRV GPUハンドル
    };

    // テクスチャデータの配列
    std::vector<TextureData> textureDatas;

    // SRVインデックスの開始番号（ImGui が 0 番を使うので 1 から）
    static uint32_t kSRVIndexTop;

    // シングルトンインスタンス
    static TextureManager* instance;

    // DirectXCommon（借りるだけ。new/delete はしない）
    DirectXCommon* dxCommon_ = nullptr;
};
