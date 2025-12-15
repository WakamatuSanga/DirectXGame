#include "Object3d.h"
#include "DirectXCommon.h"
#include "TextureManager.h" // ★追加
#include <cassert>
#include <fstream>
#include <sstream>

using namespace std;
using namespace MatrixMath;

void Object3d::Initialize(Object3dCommon* object3dCommon)
{
    assert(object3dCommon);
    object3dCommon_ = object3dCommon;

    // ★Transform変数の初期化
    transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    cameraTransform_ = { {1.0f, 1.0f, 1.0f}, {0.3f, 0.0f, 0.0f}, {0.0f, 4.0f, -10.0f} };

    // 各種リソースの生成
    CreateVertexData();
    CreateMaterialResource();
    CreateTransformationMatrixResource();
    CreateDirectionalLightResource();

    // ★テクスチャの読み込み
    // .objファイルが参照しているテクスチャファイル読み込み
    TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilePath);
    // 読み込んだテクスチャの番号を取得してマテリアルデータに保存
    modelData_.material.textureIndex =
        TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData_.material.textureFilePath);
}

void Object3d::Update()
{
    // ★行列の更新処理

    // TransformからWorldMatrixを作る
    Matrix4x4 worldMatrix = MakeAffine(transform_.scale, transform_.rotate, transform_.translate);

    // CameraTransformからCameraMatrixを作る
    Matrix4x4 cameraMatrix = MakeAffine(cameraTransform_.scale, cameraTransform_.rotate, cameraTransform_.translate);

    // CameraMatrixからViewMatrixを作る (逆行列)
    Matrix4x4 viewMatrix = Inverse(cameraMatrix);

    // ProjectionMatrixを作る (透視投影)
    Matrix4x4 projectionMatrix = PerspectiveFov(0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 100.0f);

    // WVP行列を作成して定数バッファに書き込む
    transformationMatrixData_->WVP = Multipty(worldMatrix, Multipty(viewMatrix, projectionMatrix));
    transformationMatrixData_->World = worldMatrix;
}

void Object3d::Draw()
{
    // ★描画処理
    // (共通設定は Object3dCommon::CommonDrawSetting で行われている前提)

    ID3D12GraphicsCommandList* commandList = object3dCommon_->GetDxCommon()->GetCommandList();

    // 頂点バッファの設定
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

    // マテリアルCBufferの場所を設定 (RootParam[0])
    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

    // 座標変換行列CBufferの場所を設定 (RootParam[1])
    commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());

    // テクスチャSRVのDescriptorTableを設定 (RootParam[2])
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle =
        TextureManager::GetInstance()->GetSrvHandleGPU(modelData_.material.textureIndex);
    commandList->SetGraphicsRootDescriptorTable(2, textureHandle);

    // 平行光源CBufferの場所を設定 (RootParam[3])
    commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource_->GetGPUVirtualAddress());

    // 描画！ (DrawCall)
    commandList->DrawInstanced(UINT(modelData_.vertices.size()), 1, 0, 0);
}

// モデルを再読み込みしたい場合などの便利関数（必要なら使う）
void Object3d::SetModel(const std::string& directoryPath, const std::string& filename)
{
    LoadObjFile(directoryPath, filename);
    // 頂点バッファの再生成などが必要になるが、今回は簡易的にInitialize内で完結させているため
    // 本格的な差し替えを行う場合はリソース再作成ロジックの整理が必要。
    // ここでは枠組みだけ用意しておきます。
}

// ---------------------------------------------------------
//  以下、リソース生成ヘルパーなどは前回と同じ
// ---------------------------------------------------------

void Object3d::CreateVertexData()
{
    // LoadObjFile("resources/obj/fence", "fence.obj"); // ← Initialize以外で呼ぶならファイル名を引数化推奨
    LoadObjFile("resources/obj/fence", "fence.obj"); // とりあえず固定

    vertexResource_ = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * modelData_.vertices.size());
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData_.vertices.size());
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    std::memcpy(vertexData_, modelData_.vertices.data(), sizeof(VertexData) * modelData_.vertices.size());
}

void Object3d::CreateMaterialResource()
{
    materialResource_ = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->enableLighting = true; // Lighting有効
    materialData_->uvTransform = MakeIdentity4x4();
}

void Object3d::CreateTransformationMatrixResource()
{
    transformationMatrixResource_ = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));
    transformationMatrixData_->WVP = MakeIdentity4x4();
    transformationMatrixData_->World = MakeIdentity4x4();
}

void Object3d::CreateDirectionalLightResource()
{
    directionalLightResource_ = object3dCommon_->GetDxCommon()->CreateBufferResource(sizeof(DirectionalLight));
    directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));
    directionalLightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    directionalLightData_->direction = { 0.0f, -1.0f, 0.0f };
    directionalLightData_->intensity = 1.0f;
}

// ... LoadMaterialTemplateFile, LoadObjFile は前回と同じ実装 ...
// (省略せずに以前のコードを使ってください)
Object3d::MaterialData Object3d::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
    // ... 前回と同じ ...
    MaterialData materialData; // ★初期化忘れずに
    materialData.textureIndex = 0;
    std::string line;
    std::ifstream file(directoryPath + "/" + filename);
    assert(file.is_open());
    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;
        if (identifier == "map_Kd") {
            std::string textureFilename;
            s >> textureFilename;
            materialData.textureFilePath = directoryPath + "/" + textureFilename;
        }
    }
    return materialData;
}

void Object3d::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
    // ... 前回と同じ ...
    modelData_.vertices.clear();
    std::vector<Vector4> positions;
    std::vector<Vector3> normals;
    std::vector<Vector2> texcoords;
    std::string line;
    std::ifstream file(directoryPath + "/" + filename);
    assert(file.is_open());
    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;
        if (identifier == "v") {
            Vector4 p{};
            s >> p.x >> p.y >> p.z;
            p.x *= -1.0f; p.w = 1.0f;
            positions.push_back(p);
        } else if (identifier == "vt") {
            Vector2 uv{};
            s >> uv.x >> uv.y;
            uv.y = 1.0f - uv.y;
            texcoords.push_back(uv);
        } else if (identifier == "vn") {
            Vector3 n{};
            s >> n.x >> n.y >> n.z;
            n.x *= -1.0f;
            normals.push_back(n);
        } else if (identifier == "f") {
            VertexData triangle[3]{};
            for (int i = 0; i < 3; ++i) {
                std::string vertexDefinition;
                s >> vertexDefinition;
                std::istringstream v(vertexDefinition);
                uint32_t idx[3]{};
                for (int e = 0; e < 3; ++e) {
                    std::string indexStr;
                    std::getline(v, indexStr, '/');
                    idx[e] = static_cast<uint32_t>(std::stoi(indexStr));
                }
                Vector4 p = positions[idx[0] - 1];
                Vector2 t = texcoords[idx[1] - 1];
                Vector3 n = normals[idx[2] - 1];
                triangle[i] = { p, t, n };
            }
            modelData_.vertices.push_back(triangle[2]);
            modelData_.vertices.push_back(triangle[1]);
            modelData_.vertices.push_back(triangle[0]);
        } else if (identifier == "mtllib") {
            std::string materialFilename;
            s >> materialFilename;
            modelData_.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
        }
    }
}