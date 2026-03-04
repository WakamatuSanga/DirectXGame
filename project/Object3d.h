#pragma once
#include "Object3dCommon.h"
#include "Model.h"
#include "Matrix4x4.h"
#include "Camera.h"
#include <wrl.h>
#include <d3d12.h>

class Object3d {
public:
    struct TransformationMatrix {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 WorldInverseTranspose; // 非均一スケール対応
    };

    struct DirectionalLight {
        Vector4 color;
        Vector3 direction;
        float intensity;
        Vector3 cameraPosition; // 鏡面反射用カメラ座標
        float shininess;        // 光沢度
    };

public:
    void Initialize(Object3dCommon* object3dCommon);
    void Update();
    void Draw();

    void SetModel(Model* model) { model_ = model; }
    void SetCamera(Camera* camera) { camera_ = camera; }

    void SetScale(const Vector3& scale) { transform_.scale = scale; }
    void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
    void SetTranslate(const Vector3& translate) { transform_.translate = translate; }
    Transform& GetTransform() { return transform_; }

    DirectionalLight* GetDirectionalLightData() { return directionalLightData_; }

private:
    void CreateTransformationMatrixResource();
    void CreateDirectionalLightResource();

private:
    Object3dCommon* object3dCommon_ = nullptr;
    Model* model_ = nullptr;
    Camera* camera_ = nullptr;

    Transform transform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
    TransformationMatrix* transformationMatrixData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    DirectionalLight* directionalLightData_ = nullptr;
};