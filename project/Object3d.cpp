#include "Object3d.h"
#include "DirectXCommon.h"
#include <cassert>

using namespace MatrixMath;

void Object3d::Initialize(Object3dCommon* object3dCommon) {
    assert(object3dCommon);
    object3dCommon_ = object3dCommon;

    transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

    CreateTransformationMatrixResource();
    CreateDirectionalLightResource();
    CreateCameraResource(); // ★追加
}

void Object3d::Update() {
    Matrix4x4 worldMatrix = MakeAffine(transform_.scale, transform_.rotate, transform_.translate);

    if (camera_) {
        const Matrix4x4& viewProjection = camera_->GetViewProjectionMatrix();
        transformationMatrixData_->WVP = Multipty(worldMatrix, viewProjection);

        // ★追加: カメラ座標をGPUへ送る
        cameraData_->worldPosition = camera_->GetTranslate();
    } else {
        transformationMatrixData_->WVP = worldMatrix;
    }

    transformationMatrixData_->World = worldMatrix;
}

void Object3d::Draw() {
    ID3D12GraphicsCommandList* commandList = object3dCommon_->GetDxCommon()->GetCommandList();

    commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource_->GetGPUVirtualAddress());
    // ★追加: Camera用CBV (b2)
    commandList->SetGraphicsRootConstantBufferView(4, cameraResource_->GetGPUVirtualAddress());

    if (model_) {
        model_->Draw();
    } else if (sphere_) {
        sphere_->Draw();
    }
}

void Object3d::CreateTransformationMatrixResource() {
    transformationMatrixResource_ = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));
    transformationMatrixData_->WVP = MakeIdentity4x4();
    transformationMatrixData_->World = MakeIdentity4x4();
}

void Object3d::CreateDirectionalLightResource() {
    directionalLightResource_ = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(DirectionalLight));
    directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));
    directionalLightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    directionalLightData_->direction = { 0.0f, 0.0f, 1.0f };
    directionalLightData_->intensity = 1.0f;
}

void Object3d::CreateCameraResource() {
    // 256バイトアライメント
    size_t sizeInBytes = (sizeof(CameraForGPU) + 255) & ~255;
    cameraResource_ = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeInBytes);
    cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));
    cameraData_->worldPosition = { 0.0f, 0.0f, 0.0f };
}