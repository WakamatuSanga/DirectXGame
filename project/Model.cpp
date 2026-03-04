#include "Model.h"
#include "TextureManager.h"
#include <cassert>
#include <fstream>
#include <sstream>
#include <cmath>

using namespace std;
using namespace MatrixMath;

void Model::Initialize(ModelCommon* modelCommon, const std::string& directoryPath, const std::string& filename) {
    ModelData data = LoadObjFile(directoryPath, filename);
    Initialize(modelCommon, data);
}

void Model::Initialize(ModelCommon* modelCommon, const ModelData& modelData) {
    assert(modelCommon);
    modelCommon_ = modelCommon;
    modelData_ = modelData;

    // --- 頂点バッファ作成 ---
    vertexResource_ = modelCommon_->GetDxCommon()->CreateBufferResource(
        sizeof(VertexData) * modelData_.vertices.size());

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData_.vertices.size());
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    std::memcpy(vertexData_, modelData_.vertices.data(), sizeof(VertexData) * modelData_.vertices.size());

    // --- マテリアルリソース作成 ---
    materialResource_ = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->enableLighting = 1;
    materialData_->uvTransform = MakeIdentity4x4();
}

void Model::Draw() {
    ID3D12GraphicsCommandList* commandList = modelCommon_->GetDxCommon()->GetCommandList();

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

    // ★修正: 最新の textureIndex を使って描画する
    commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(modelData_.material.textureIndex));

    commandList->DrawInstanced(static_cast<UINT>(modelData_.vertices.size()), 1, 0, 0);
}

Model::ModelData Model::CreateSphereData(uint32_t subdivision) {
    ModelData data;

    // ★修正: 初期値として、確実に存在する「uvChecker.png」のインデックスを取得してセットしておく
    data.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/obj/axis/uvChecker.png");

    const float pi = 3.14159265359f;

    for (uint32_t lat = 0; lat < subdivision; ++lat) {
        float latAngle0 = pi * (float)lat / subdivision;
        float latAngle1 = pi * (float)(lat + 1) / subdivision;

        float y0 = std::cos(latAngle0);
        float r0 = std::sin(latAngle0);
        float y1 = std::cos(latAngle1);
        float r1 = std::sin(latAngle1);

        for (uint32_t lon = 0; lon < subdivision; ++lon) {
            float lonAngle0 = 2.0f * pi * (float)lon / subdivision;
            float lonAngle1 = 2.0f * pi * (float)(lon + 1) / subdivision;

            float u0 = (float)lon / subdivision;
            float u1 = (float)(lon + 1) / subdivision;
            float v0 = (float)lat / subdivision;
            float v1 = (float)(lat + 1) / subdivision;

            VertexData v00 = { {r0 * std::cos(lonAngle0), y0, r0 * std::sin(lonAngle0), 1.0f}, {u0, v0}, {r0 * std::cos(lonAngle0), y0, r0 * std::sin(lonAngle0)} };
            VertexData v10 = { {r1 * std::cos(lonAngle0), y1, r1 * std::sin(lonAngle0), 1.0f}, {u0, v1}, {r1 * std::cos(lonAngle0), y1, r1 * std::sin(lonAngle0)} };
            VertexData v01 = { {r0 * std::cos(lonAngle1), y0, r0 * std::sin(lonAngle1), 1.0f}, {u1, v0}, {r0 * std::cos(lonAngle1), y0, r0 * std::sin(lonAngle1)} };
            VertexData v11 = { {r1 * std::cos(lonAngle1), y1, r1 * std::sin(lonAngle1), 1.0f}, {u1, v1}, {r1 * std::cos(lonAngle1), y1, r1 * std::sin(lonAngle1)} };

            data.vertices.push_back(v00);
            data.vertices.push_back(v01);
            data.vertices.push_back(v10);
            data.vertices.push_back(v01);
            data.vertices.push_back(v11);
            data.vertices.push_back(v10);
        }
    }
    return data;
}

Model::MaterialData Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
    MaterialData materialData;
    std::string line;
    std::ifstream file(directoryPath + "/" + filename);
    if (!file.is_open()) return materialData;

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

Model::ModelData Model::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
    ModelData modelData;
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
            p.w = 1.0f;
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
                p.x *= -1.0f;
                triangle[i] = { p, t, n };
            }
            modelData.vertices.push_back(triangle[2]);
            modelData.vertices.push_back(triangle[1]);
            modelData.vertices.push_back(triangle[0]);
        } else if (identifier == "mtllib") {
            std::string materialFilename;
            s >> materialFilename;
            modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
        }
    }
    return modelData;
}