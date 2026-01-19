#pragma once
#include "Object3dCommon.h"
#include "Model.h" // Modelクラスを知る必要がある
#include "Matrix4x4.h"
#include <wrl.h>
#include <d3d12.h>

class Object3d {
public: // 構造体定義
    // 座標変換行列データ
    struct TransformationMatrix {
        Matrix4x4 WVP;
        Matrix4x4 World;
    };

    // 平行光源データ
    struct DirectionalLight {
        Vector4 color;
        Vector3 direction;
        float intensity;
    };

public: // メンバ関数
    void Initialize(Object3dCommon* object3dCommon);
    void Update();
    void Draw();

    // セッター
    void SetModel(Model* model) { model_ = model; }

    // Transform / Camera
    void SetScale(const Vector3& scale) { transform_.scale = scale; }
    void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
    void SetTranslate(const Vector3& translate) { transform_.translate = translate; }

    // ImGui等から直接いじれるように非const参照を返す
    Transform& GetTransform() { return transform_; }

    void SetCameraTransform(const Transform& cameraTransform) { cameraTransform_ = cameraTransform; }

    // ★ImGui用：ライトのデータを直接変更できるようにする
    DirectionalLight* GetDirectionalLightData() { return directionalLightData_; }

private:
    void CreateTransformationMatrixResource();
    void CreateDirectionalLightResource();

private:
    Object3dCommon* object3dCommon_ = nullptr;
    Model* model_ = nullptr;

    Transform transform_{ {1,1,1}, {0,0,0}, {0,0,0} };
    Transform cameraTransform_{ {1,1,1}, {0,0,0}, {0,0,-5} };

    // 座標変換行列リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
    TransformationMatrix* transformationMatrixData_ = nullptr;

    // 平行光源リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    DirectionalLight* directionalLightData_ = nullptr;
};