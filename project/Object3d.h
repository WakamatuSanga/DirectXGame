#pragma once
#include "Object3dCommon.h"
#include "Model.h"
#include "Sphere.h" // Sphereも含める
#include "Matrix4x4.h"
#include "Camera.h"
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

    // ★追加: カメラ座標をGPUに送る構造体
    struct CameraForGPU {
        Vector3 worldPosition;
    };

public:
    void Initialize(Object3dCommon* object3dCommon);
    void Update();
    void Draw();

    void SetModel(Model* model) { model_ = model; sphere_ = nullptr; }
    void SetSphere(Sphere* sphere) { sphere_ = sphere; model_ = nullptr; }
    void SetCamera(Camera* camera) { camera_ = camera; }

    void SetScale(const Vector3& scale) { transform_.scale = scale; }
    void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
    void SetTranslate(const Vector3& translate) { transform_.translate = translate; }
    Transform& GetTransform() { return transform_; }

    DirectionalLight* GetDirectionalLightData() { return directionalLightData_; }

private:
    void CreateTransformationMatrixResource();
    void CreateDirectionalLightResource();
    void CreateCameraResource(); // ★追加

private:
    Object3dCommon* object3dCommon_ = nullptr;
    Model* model_ = nullptr;
    Sphere* sphere_ = nullptr;
    Camera* camera_ = nullptr;

    Transform transform_{ {1,1,1}, {0,0,0}, {0,0,0} };

    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
    TransformationMatrix* transformationMatrixData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    DirectionalLight* directionalLightData_ = nullptr;

    // ★追加
    Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
    CameraForGPU* cameraData_ = nullptr;
};