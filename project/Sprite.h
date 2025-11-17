#pragma once
#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

#include "Matrix4x4.h"   // ★ここを追加（Vector/Matrix/Transform用）

class SpriteCommon;

// スプライト1枚分
class Sprite {
public:
    void Initialize(SpriteCommon* spriteCommon);
    void Update();
    void Draw();

    // 位置・スケールを外から変えたいとき用（必要になったら使う）
    void SetPosition(float x, float y) {
        transform_.translate.x = x;
        transform_.translate.y = y;
    }
    void SetScale(float sx, float sy) {
        transform_.scale.x = sx;
        transform_.scale.y = sy;
    }

    // main 側で作ったテクスチャ SRV を渡す用
    void SetTextureHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) {
        textureSrvHandleGPU_ = handle;
    }

private:
    SpriteCommon* spriteCommon_ = nullptr;

    struct VertexData {
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    VertexData* vertexData_ = nullptr;
    uint32_t* indexData_ = nullptr;

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    D3D12_INDEX_BUFFER_VIEW  indexBufferView_{};

    struct Material {
        Vector4   color;
        int32_t   enableLighting;
        float     padding[3];
        Matrix4x4 uvTransform;
    };
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;

    struct TransformationMatrix {
        Matrix4x4 WVP;
        Matrix4x4 World;
    };
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
    TransformationMatrix* transformationMatrixData_ = nullptr;

    Transform transform_
    { {1.0f,1.0f,1.0f},
      {0.0f,0.0f,0.0f},
      {0.0f,0.0f,0.0f} };

    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};  // t0
};
