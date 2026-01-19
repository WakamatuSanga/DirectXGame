#pragma once
#include "ModelCommon.h"
#include "Matrix4x4.h" // VertexData等のために必要
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

    struct Material {
        Vector4 color;
        int32_t enableLighting;
        float padding[3];
        Matrix4x4 uvTransform;
    };

public: // メンバ関数
    // 初期化
    void Initialize(ModelCommon* modelCommon, const std::string& directoryPath, const std::string& filename);

    // 描画
    void Draw();

    // ★ImGui用：マテリアルデータをいじれるようにする
    Material* GetMaterialData() { return materialData_; }

    // ★ImGui用：テクスチャを差し替えられるようにする
    void SetTextureIndex(uint32_t index) { modelData_.material.textureIndex = index; }

    // .mtlファイルの読み取り (静的関数)
    static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

    // .objファイルの読み取り (静的関数)
    static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

private: // メンバ変数
    ModelCommon* modelCommon_ = nullptr;

    // Objファイルから読み込んだデータ
    ModelData modelData_;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    VertexData* vertexData_ = nullptr;

    // マテリアルリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;
};