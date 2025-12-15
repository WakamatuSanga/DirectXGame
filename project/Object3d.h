#pragma once
#include "Object3dCommon.h"
#include "Matrix4x4.h"
#include <string>
#include <vector>
#include <wrl.h>
#include <d3d12.h>

class Object3d
{
public: // サブクラス・構造体定義
    // 頂点データ
    struct VertexData {
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    // マテリアルデータ
    struct MaterialData {
        std::string textureFilePath;
        uint32_t textureIndex = 0; // ★追加：テクスチャ番号
    };

    // モデルデータ
    struct ModelData {
        std::vector<VertexData> vertices;
        MaterialData material;
    };

    // 座標変換行列データ
    struct TransformationMatrix {
        Matrix4x4 WVP;
        Matrix4x4 World;
    };

    // マテリアル定数バッファ用データ
    struct Material {
        Vector4 color;
        int32_t enableLighting;
        float padding[3];
        Matrix4x4 uvTransform;
    };

    // 平行光源定数バッファ用データ
    struct DirectionalLight {
        Vector4 color;
        Vector3 direction;
        float intensity;
    };

public: // メンバ関数
    // 初期化
    void Initialize(Object3dCommon* object3dCommon);

    // 更新
    void Update();

    // 描画
    void Draw();

    // ★セッター・ゲッター（main.cppから操作するために追加）
    void SetModel(const std::string& directoryPath, const std::string& filename); // モデル差し替え用

    // Transform
    void SetScale(const Vector3& scale) { transform_.scale = scale; }
    void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
    void SetTranslate(const Vector3& translate) { transform_.translate = translate; }
    const Transform& GetTransform() const { return transform_; }

    // Camera
    void SetCameraTransform(const Transform& cameraTransform) { cameraTransform_ = cameraTransform; }
    const Transform& GetCameraTransform() const { return cameraTransform_; }

private: // メンバ関数 (内部処理用)
    static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
    void LoadObjFile(const std::string& directoryPath, const std::string& filename);

    void CreateVertexData();
    void CreateMaterialResource();
    void CreateTransformationMatrixResource();
    void CreateDirectionalLightResource();

private: // メンバ変数
    Object3dCommon* object3dCommon_ = nullptr;

    // モデルデータ
    ModelData modelData_;

    // ★Transform情報
    Transform transform_;
    Transform cameraTransform_;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    VertexData* vertexData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
    TransformationMatrix* transformationMatrixData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    DirectionalLight* directionalLightData_ = nullptr;
};