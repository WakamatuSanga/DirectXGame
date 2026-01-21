#pragma once
#include "Object3dCommon.h"
#include "Model.h"
#include "Matrix4x4.h"
#include "Camera.h" // 追加
#include <wrl.h>
#include <d3d12.h>

class Object3d {
public:
    struct TransformationMatrix {
        Matrix4x4 WVP;
        Matrix4x4 World;
    };

    struct DirectionalLight {
        Vector4 color;
        Vector3 direction;
        float intensity;
    };

public:
    void Initialize(Object3dCommon* object3dCommon);
    void Update();
    void Draw();

    void SetModel(Model* model) { model_ = model; }
    void SetCamera(Camera* camera) { camera_ = camera; } // カメラセット

    void SetScale(const Vector3& scale) { transform_.scale = scale; }
    void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
    void SetTranslate(const Vector3& translate) { transform_.translate = translate; }
    Transform& GetTransform() { return transform_; } // 参照返し(ImGui用)

    DirectionalLight* GetDirectionalLightData() { return directionalLightData_; }

private:
    void CreateTransformationMatrixResource();
    void CreateDirectionalLightResource();

private:
    Object3dCommon* object3dCommon_ = nullptr;
    Model* model_ = nullptr;
    Camera* camera_ = nullptr; // カメラポインタ

    Transform transform_{ {1,1,1}, {0,0,0}, {0,0,0} };

    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
    TransformationMatrix* transformationMatrixData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    DirectionalLight* directionalLightData_ = nullptr;
};