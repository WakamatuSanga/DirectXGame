#include "Object3d.h"
#include "DirectXCommon.h"
#include <cassert>

using namespace MatrixMath;

void Object3d::Initialize(Object3dCommon* object3dCommon) {
    assert(object3dCommon);
    object3dCommon_ = object3dCommon;

    // Transform初期化
    transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    cameraTransform_ = { {1.0f, 1.0f, 1.0f}, {0.3f, 0.0f, 0.0f}, {0.0f, 4.0f, -10.0f} };

    // リソース生成 (モデル関連は削除し、座標・ライトのみ作成)
    CreateTransformationMatrixResource();
    CreateDirectionalLightResource();
}

void Object3d::Update() {
    // 行列計算
    Matrix4x4 worldMatrix = MakeAffine(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 cameraMatrix = MakeAffine(cameraTransform_.scale, cameraTransform_.rotate, cameraTransform_.translate);
    Matrix4x4 viewMatrix = Inverse(cameraMatrix);
    Matrix4x4 projectionMatrix = PerspectiveFov(0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 100.0f);

    transformationMatrixData_->WVP = Multipty(worldMatrix, Multipty(viewMatrix, projectionMatrix));
    transformationMatrixData_->World = worldMatrix;
}

void Object3d::Draw() {
    ID3D12GraphicsCommandList* commandList = object3dCommon_->GetDxCommon()->GetCommandList();

    // 座標変換行列CBuffer (RootParam[1])
    commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());

    // 平行光源CBuffer (RootParam[3])
    commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource_->GetGPUVirtualAddress());

    // Modelがセットされていれば描画
    if (model_) {
        model_->Draw();
    }
}

// ---------------------------------------------------
// 以下のリソース生成関数は前回と同じ (座標・ライト用)
// ---------------------------------------------------

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
    directionalLightData_->direction = { 0.0f, -1.0f, 0.0f };
    directionalLightData_->intensity = 1.0f;
}