#pragma once
#include "ModelCommon.h"
#include "Matrix4x4.h"
#include <string>
#include <vector>
#include <wrl.h>
#include <d3d12.h>

class Model {
public:
    struct VertexData {
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    struct MaterialData {
        std::string textureFilePath;
        uint32_t textureIndex = 0;
    };

    struct ModelData {
        std::vector<VertexData> vertices;
        MaterialData material;
    };

    // 定数バッファ用マテリアル
    struct Material {
        Vector4 color;
        int32_t enableLighting;
        float padding[3];
        Matrix4x4 uvTransform;
        float shininess; // ★スライド通り: 光沢度を追加
    };

public:
    // 初期化
    void Initialize(ModelCommon* modelCommon, const std::string& directoryPath, const std::string& filename);

    // 描画
    void Draw();

    // 外部からテクスチャを切り替えるための関数
    void SetTextureIndex(uint32_t index) { modelData_.material.textureIndex = index; }

    // マテリアルデータへのアクセス
    Material* GetMaterialData() { return materialData_; }

    // 静的関数
    static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
    static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

private:
    ModelCommon* modelCommon_ = nullptr;
    ModelData modelData_;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    VertexData* vertexData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;
};