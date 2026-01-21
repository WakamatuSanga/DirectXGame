#include "Sphere.h"
#include <cmath>
#include <numbers>
#include <cassert>

using namespace MatrixMath;

void Sphere::Initialize(DirectXCommon* dxCommon) {
    assert(dxCommon);
    dxCommon_ = dxCommon;

    // 頂点データの生成
    CreateVertexData();

    // バッファの生成
    CreateBuffers();

    // デフォルトテクスチャ
    TextureManager::GetInstance()->LoadTexture("resources/obj/monsterBall/monsterBall.png");
    textureIndex_ = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/obj/monsterBall/monsterBall.png");
}

void Sphere::CreateVertexData() {
    vertices_.clear();

    // 緯度方向の分割ループ (lat)
    for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
        for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
            float lat0 = (float)latIndex * std::numbers::pi_v<float> / kSubdivision;
            float lat1 = (float)(latIndex + 1) * std::numbers::pi_v<float> / kSubdivision;

            float lon0 = (float)lonIndex * 2.0f * std::numbers::pi_v<float> / kSubdivision;
            float lon1 = (float)(lonIndex + 1) * 2.0f * std::numbers::pi_v<float> / kSubdivision;

            auto GetVertex = [&](float theta, float phi) {
                VertexData v;
                // 座標 (半径1.0f)
                v.position.x = std::cos(phi) * std::sin(theta);
                v.position.y = std::cos(theta);
                v.position.z = std::sin(phi) * std::sin(theta);
                v.position.w = 1.0f;

                // 法線 (単位球なので座標と同じ方向)
                v.normal.x = v.position.x;
                v.normal.y = v.position.y;
                v.normal.z = v.position.z;

                // UV
                v.texcoord.x = 1.0f - (phi / (2.0f * std::numbers::pi_v<float>));
                v.texcoord.y = theta / std::numbers::pi_v<float>;
                return v;
                };

            VertexData a = GetVertex(lat0, lon0);
            VertexData b = GetVertex(lat0, lon1);
            VertexData c = GetVertex(lat1, lon0);
            VertexData d = GetVertex(lat1, lon1);

            // Tri 1: a, c, b
            vertices_.push_back(a);
            vertices_.push_back(c);
            vertices_.push_back(b);

            // Tri 2: b, c, d
            vertices_.push_back(b);
            vertices_.push_back(c);
            vertices_.push_back(d);
        }
    }
}

void Sphere::CreateBuffers() {
    // 頂点バッファ生成
    size_t sizeInBytes = sizeof(VertexData) * vertices_.size();
    vertexResource_ = dxCommon_->CreateBufferResource(sizeInBytes);

    // VBV設定
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeInBytes);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    // データ転送
    VertexData* vertMap = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertMap));
    std::copy(vertices_.begin(), vertices_.end(), vertMap);
    vertexResource_->Unmap(0, nullptr);

    // マテリアルリソース生成
    materialResource_ = dxCommon_->CreateBufferResource(sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    // マテリアル初期値
    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->enableLighting = true; // ライティング有効
    materialData_->uvTransform = MatrixMath::MakeIdentity4x4();
    materialData_->shininess = 40.0f;     // 初期光沢度
    materialData_->lightingType = 2;      // 0:HalfLambert, 1:Lambert, 2:Phong, 3:BlinnPhong
}

void Sphere::Draw() {
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    // 頂点バッファをセット
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

    // マテリアルCBuffer (RootParam[0])
    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

    // テクスチャSRV (RootParam[2])
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle =
        TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex_);
    commandList->SetGraphicsRootDescriptorTable(2, textureHandle);

    // 描画
    commandList->DrawInstanced(static_cast<UINT>(vertices_.size()), 1, 0, 0);
}

void Sphere::SetTexture(const std::string& filePath) {
    TextureManager::GetInstance()->LoadTexture(filePath);
    textureIndex_ = TextureManager::GetInstance()->GetTextureIndexByFilePath(filePath);
}