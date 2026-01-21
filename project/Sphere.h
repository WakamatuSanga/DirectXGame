#pragma once
#include "DirectXCommon.h"
#include "TextureManager.h"
#include "Matrix4x4.h"
#include <vector>
#include <wrl.h>
#include <string>

class Sphere {
public:
    struct VertexData {
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    // マテリアル (定数バッファ用)
    struct Material {
        Vector4 color;
        int32_t enableLighting; // 0:なし, 1:あり
        float padding[3];
        Matrix4x4 uvTransform;
        float shininess;        // 光沢度
        int32_t lightingType;   // 0:HalfLambert, 1:Lambert, 2:Phong, 3:BlinnPhong
        float padding2[2];      // パディング調整
    };

public:
    // 初期化 (分割数kSubdivisionを指定)
    void Initialize(DirectXCommon* dxCommon);

    // 描画
    void Draw();

    // テクスチャ設定
    void SetTexture(const std::string& filePath);
    void SetTextureIndex(uint32_t index) { textureIndex_ = index; }

    // マテリアル取得
    Material* GetMaterialData() { return materialData_; }

private:
    // 頂点データ生成
    void CreateVertexData();
    // リソース作成
    void CreateBuffers();

private:
    DirectXCommon* dxCommon_ = nullptr;

    // 分割数 (経度・緯度方向)
    const uint32_t kSubdivision = 16;

    // データ
    std::vector<VertexData> vertices_;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    // マテリアルリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;

    // テクスチャ
    uint32_t textureIndex_ = 0;
};