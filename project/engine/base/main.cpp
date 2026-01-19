#define _USE_MATH_DEFINES

// --- Windows / 標準ライブラリ ---
#include <Windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <strsafe.h>
#include <vector>
#include <random>
#include <sstream>
#include <iostream>
#include <format>
#include <algorithm>
#include <cmath>

// --- Direct3D 12 / DXGI 関連 ---
#include <d3d12.h>
#include <dxgi1_6.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

// --- DirectX デバッグ支援 ---
#include <dbghelp.h>
#include <dxgidebug.h>
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "dxguid.lib")

// --- DXC (Shader Compiler) ---
#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")

// --- DirectXTex ---
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

// --- ImGui ---
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

// --- 自作ヘッダ ---
#include "Input.h"
#include "WinApp.h"
#include "DirectXCommon.h"
#include "SpriteCommon.h"
#include "Sprite.h"
#include "Object3dCommon.h"
#include "Object3d.h"
#include "Matrix4x4.h"
#include "Logger.h"
#include "StringUtility.h"
#include "TextureManager.h"
#include "ModelCommon.h"
#include "Model.h"
#include "ModelManager.h" 

// ------------- 型定義 / 構造体など -------------

using namespace MatrixMath;

// パーティクル用定数
const uint32_t kNumInstance = 10;

// パーティクル用構造体
struct Particle {
    Transform transform;
    Vector3   velocity;
};

// パーティクル用の変換行列構造体
struct ParticleTransformationMatrix {
    Matrix4x4 WVP;
    Matrix4x4 World;
};

// マテリアル構造体（パーティクル用）
struct ParticleMaterial {
    Vector4   color;
    int32_t   enableLighting;
    float     padding[3];
    Matrix4x4 uvTransform;
};

// クラッシュ時に dmp を出す
static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
    SYSTEMTIME time;
    GetLocalTime(&time);
    wchar_t filePath[MAX_PATH] = { 0 };
    CreateDirectory(L"./Dumps", nullptr);
    StringCchPrintfW(
        filePath, MAX_PATH,
        L"./Dumps/%04d-%02d%02d-%02d%02d.dmp",
        time.wYear, time.wMonth, time.wDay,
        time.wHour, time.wMinute);

    HANDLE dumpFileHandle = CreateFile(
        filePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        0, CREATE_ALWAYS, 0, 0);

    DWORD processId = GetCurrentProcessId();
    DWORD threadId = GetCurrentThreadId();

    MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{};
    minidumpInformation.ThreadId = threadId;
    minidumpInformation.ExceptionPointers = exception;
    minidumpInformation.ClientPointers = TRUE;

    MiniDumpWriteDump(
        GetCurrentProcess(),
        processId,
        dumpFileHandle,
        MiniDumpNormal,
        &minidumpInformation,
        nullptr, nullptr);

    return EXCEPTION_EXECUTE_HANDLER;
}

// ログ
void LogToStream(std::ostream& os, const std::string& message) {
    os << message << std::endl;
    Logger::Log(message);
}

// 正規化
Vector3 Normalize(const Vector3& v) {
    float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length == 0.0f) {
        return { 0.0f,0.0f,0.0f };
    }
    return { v.x / length, v.y / length, v.z / length };
}

// 頂点/定数バッファ用リソース生成ヘルパー
ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {
    D3D12_HEAP_PROPERTIES uploadHeapProperties{};
    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = sizeInBytes;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ID3D12Resource* resource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&resource));
    assert(SUCCEEDED(hr));
    return resource;
}

// ------------------------
// WinMain
// ------------------------

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    CoInitializeEx(0, COINIT_MULTITHREADED);
    SetUnhandledExceptionFilter(ExportDump);

    std::filesystem::create_directory("logs");

#pragma region 基盤初期化
    WinApp* winApp = new WinApp();
    winApp->Initialize();

    DirectXCommon* dxCommon = new DirectXCommon();
    dxCommon->Initialize(winApp);

    TextureManager* texManager = TextureManager::GetInstance();
    texManager->Initialize(dxCommon);

    SpriteCommon* spriteCommon = new SpriteCommon();
    spriteCommon->Initialize(dxCommon);

    Object3dCommon* object3dCommon = new Object3dCommon();
    object3dCommon->Initialize(dxCommon);

    ModelManager* modelManager = ModelManager::GetInstance();
    modelManager->Initialize(dxCommon);

    Input input;
    input.Initialize(winApp);

    ID3D12Device* device = dxCommon->GetDevice();
    ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();
    ID3D12DescriptorHeap* srvDescriptorHeap = dxCommon->GetSrvDescriptorHeap();
#pragma endregion

    // ==========================================================
    // リソース読み込み・生成
    // ==========================================================

    // --- モデル読み込み ---
    modelManager->LoadModel("resources/obj/fence/fence.obj");
    Model* modelFence = modelManager->FindModel("resources/obj/fence/fence.obj");

    // --- 3Dオブジェクト生成 ---
    Object3d* object3d = new Object3d();
    object3d->Initialize(object3dCommon);
    object3d->SetModel(modelFence);
    object3d->SetTranslate({ 0.0f, 0.0f, 0.0f });

    // --- テクスチャ読み込み ---
    texManager->LoadTexture("resources/obj/axis/uvChecker.png");
    uint32_t texIndexUvChecker = texManager->GetTextureIndexByFilePath("resources/obj/axis/uvChecker.png");

    // fenceモデルが持っているテクスチャパスを取得してインデックス確保
    // (Modelクラスがロード時にやってくれているが、ImGuiで切り替えるためにここでも把握しておく)
    // 今回は簡易的に0:uvChecker, 1:fence標準テクスチャと仮定して切り替える
    // 本来はModelからテクスチャIDを取得するべき

    // ==========================================================
    // パーティクル初期化 (現状 main.cpp で管理)
    // ==========================================================

    ID3D12Resource* vertexResourceSprite = CreateBufferResource(device, sizeof(Model::VertexData) * 4);
    Model::VertexData* vertexDataSprite = nullptr;
    vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
    vertexDataSprite[0] = { { -0.5f, -0.5f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } };
    vertexDataSprite[1] = { { -0.5f,  0.5f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } };
    vertexDataSprite[2] = { {  0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } };
    vertexDataSprite[3] = { {  0.5f,  0.5f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } };

    D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
    vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
    vertexBufferViewSprite.SizeInBytes = sizeof(Model::VertexData) * 4;
    vertexBufferViewSprite.StrideInBytes = sizeof(Model::VertexData);

    ID3D12Resource* indexResourceSprite = CreateBufferResource(device, sizeof(uint32_t) * 6);
    uint32_t* indexDataSprite = nullptr;
    indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
    indexDataSprite[0] = 0; indexDataSprite[1] = 1; indexDataSprite[2] = 2;
    indexDataSprite[3] = 2; indexDataSprite[4] = 1; indexDataSprite[5] = 3;

    D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
    indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
    indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
    indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

    ID3D12Resource* materialResourceSprite = CreateBufferResource(device, sizeof(ParticleMaterial));
    ParticleMaterial* materialDataSprite = nullptr;
    materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
    materialDataSprite->color = { 1.0f,1.0f,1.0f,1.0f };
    materialDataSprite->uvTransform = MakeIdentity4x4();
    materialDataSprite->enableLighting = false;

    ID3D12Resource* instancingResource = CreateBufferResource(device, sizeof(ParticleTransformationMatrix) * kNumInstance);
    ParticleTransformationMatrix* instancingData = nullptr;
    instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&instancingData));
    for (uint32_t i = 0; i < kNumInstance; ++i) {
        instancingData[i].WVP = MakeIdentity4x4();
        instancingData[i].World = MakeIdentity4x4();
    }

    const uint32_t kInstancingSrvIndex = 3;
    D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
    instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    instancingSrvDesc.Buffer.FirstElement = 0;
    instancingSrvDesc.Buffer.NumElements = kNumInstance;
    instancingSrvDesc.Buffer.StructureByteStride = sizeof(ParticleTransformationMatrix);
    instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvCPU = dxCommon->GetSRVCPUDescriptorHandle(kInstancingSrvIndex);
    D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvGPU = dxCommon->GetSRVGPUDescriptorHandle(kInstancingSrvIndex);
    device->CreateShaderResourceView(instancingResource, &instancingSrvDesc, instancingSrvCPU);


    // --- ImGui用変数群 ---
    Transform cameraTransform{ {1,1,1}, {0.3f,0,0}, {0,4.0f,-10.0f} };

    // パーティクル制御用
    float layoutStartX = -1.4f;
    float layoutStartY = -0.8f;
    float layoutStartZ = 0.0f;
    float layoutStepX = 0.22f;
    float layoutStepY = 0.11f;
    float layoutStepZ = 0.05f;
    float spriteScaleXY = 0.90f;
    float randPosRange = 0.15f;
    float randVelRange = 1.0f;
    float randVelZScale = 0.0f;

    Particle particles[kNumInstance];
    std::random_device seedGenerator;
    std::mt19937 randomEngine(seedGenerator());

    // パーティクル初期配置
    for (uint32_t i = 0; i < kNumInstance; ++i) {
        particles[i].transform.scale = { 1.0f,1.0f,1.0f };
        particles[i].transform.rotate = { 0.0f,0.0f,0.0f };
        particles[i].transform.translate = {
            -1.8f + 0.4f * i,
            -1.0f + 0.2f * i,
             0.25f * i
        };
        particles[i].velocity = { 0.0f,0.0f,0.0f };
    }

    // テクスチャ切り替え用
    int currentModelTexture = 0; // 0:uvChecker, 1:Fence
    // モデル読み込み時にTextureManagerに登録されたインデックスを探す必要がある
    // Fenceテクスチャは "resources/obj/fence/fence.png" と仮定して検索する
    // ※実際はLoadObjFileの結果を見るべきだが、ここでは簡易的に実装
    texManager->LoadTexture("resources/obj/fence/fence.png");
    uint32_t texIndexFence = texManager->GetTextureIndexByFilePath("resources/obj/fence/fence.png");

    // ブレンドモード用
    int currentBlendMode = 0;
    const char* blendModeNames[] = {
        "Normal","Add","Subtract","Multiply","Screen","None"
    };

    // UV Transform用
    Transform uvTransformSprite{ {1,1,1},{0,0,0},{0,0,0} };

    // ----------------
    // メインループ
    // ----------------
    while (true) {
        if (winApp->ProcessMessage()) {
            break;
        }

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        input.Update();

        // ==== カメラ操作 ====
        const bool uiWantsMouse = ImGui::GetIO().WantCaptureMouse;
        if (!uiWantsMouse && input.MouseDown(Input::MouseLeft)) {
            const float sensitivity = 0.0025f;
            cameraTransform.rotate.y += input.MouseDeltaX() * sensitivity;
            cameraTransform.rotate.x += input.MouseDeltaY() * sensitivity;
        }
        Vector3 move{ 0.0f,0.0f,0.0f };
        if (input.PushKey(DIK_W)) move.z += 1.0f;
        if (input.PushKey(DIK_S)) move.z -= 1.0f;
        if (input.PushKey(DIK_D)) move.x += 1.0f;
        if (input.PushKey(DIK_A)) move.x -= 1.0f;

        if (float len = std::sqrt(move.x * move.x + move.z * move.z); len > 0.0f) {
            move.x /= len; move.z /= len;
        }
        float speed = input.PushKey(DIK_LSHIFT) ? 6.0f : 2.5f;
        const float dt = 1.0f / 60.0f;
        if (move.x != 0.0f || move.z != 0.0f) {
            const float yaw = cameraTransform.rotate.y;
            Vector3 forward = { std::sin(yaw), 0.0f, std::cos(yaw) };
            Vector3 right = { std::cos(yaw), 0.0f,-std::sin(yaw) };
            Vector3 delta{ right.x * move.x + forward.x * move.z, 0.0f, right.z * move.x + forward.z * move.z };
            cameraTransform.translate.x += delta.x * speed * dt;
            cameraTransform.translate.y += delta.y * speed * dt;
            cameraTransform.translate.z += delta.z * speed * dt;
        }

        object3d->SetCameraTransform(cameraTransform);

        // ==== ImGui ====
        ImGui::ShowDemoWindow();
        ImGui::Begin("Material / Particle / Texture");

        ImGui::SeparatorText("cameraKey WASD Mouse");

        // Model Transform
        ImGui::SeparatorText("Model Transform");
        // Object3dから参照でTransformを取得して直接ImGuiでいじる
        Transform& tf = object3d->GetTransform();
        ImGui::SliderFloat3("Scale", &tf.scale.x, 0.1f, 5.0f);
        ImGui::SliderAngle("RotateX", &tf.rotate.x, -180.0f, 180.0f);
        ImGui::SliderAngle("RotateY", &tf.rotate.y, -180.0f, 180.0f);
        ImGui::SliderAngle("RotateZ", &tf.rotate.z, -180.0f, 180.0f);
        ImGui::SliderFloat3("Translate", &tf.translate.x, -5.0f, 5.0f);

        // Model Texture
        ImGui::SeparatorText("Model Texture");
        const char* modelTextureNames[] = { "uvChecker", "FenceTexture" };
        if (ImGui::Combo("ModelTex", &currentModelTexture, modelTextureNames, IM_ARRAYSIZE(modelTextureNames))) {
            uint32_t selectedTex = (currentModelTexture == 0) ? texIndexUvChecker : texIndexFence;
            // Modelのマテリアルを書き換え
            modelFence->SetTextureIndex(selectedTex);
        }

        // Particle Layout
        ImGui::SeparatorText("Particle Layout");
        ImGui::DragFloat("layoutStartX", &layoutStartX, 0.01f, -5.0f, 5.0f);
        ImGui::DragFloat("layoutStartY", &layoutStartY, 0.01f, -5.0f, 5.0f);
        ImGui::DragFloat("layoutStartZ", &layoutStartZ, 0.01f, -5.0f, 5.0f);
        ImGui::DragFloat("layoutStepX", &layoutStepX, 0.01f, -2.0f, 2.0f);
        ImGui::DragFloat("layoutStepY", &layoutStepY, 0.01f, -2.0f, 2.0f);
        ImGui::DragFloat("layoutStepZ", &layoutStepZ, 0.01f, -2.0f, 2.0f);
        ImGui::DragFloat("SpriteScale", &spriteScaleXY, 0.01f, 0.1f, 3.0f);

        ImGui::SeparatorText("Random Init");
        ImGui::DragFloat("PosJitter (+/-)", &randPosRange, 0.005f, 0.0f, 1.0f);
        ImGui::DragFloat("VelRange (+/-)", &randVelRange, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("VelZ Scale", &randVelZScale, 0.01f, 0.0f, 2.0f);

        if (ImGui::Button("Apply Layout")) {
            for (uint32_t i = 0; i < kNumInstance; ++i) {
                particles[i].transform.translate = { layoutStartX + layoutStepX * i, layoutStartY + layoutStepY * i, layoutStartZ + layoutStepZ * i };
                particles[i].transform.scale = { spriteScaleXY, spriteScaleXY, 1.0f };
                particles[i].velocity = { 0.0f,0.0f,0.0f };
            }
        }

        static bool play = false;
        if (ImGui::Button(play ? "Stop" : "Start")) {
            play = !play;
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            for (uint32_t i = 0; i < kNumInstance; ++i) {
                float jx = randPosRange * dist(randomEngine);
                float jy = randPosRange * dist(randomEngine);
                float jz = randPosRange * dist(randomEngine);
                particles[i].transform.scale = { spriteScaleXY, spriteScaleXY, 1.0f };
                particles[i].transform.rotate = { 0.0f,0.0f,0.0f };
                particles[i].transform.translate = { layoutStartX + layoutStepX * i + jx, layoutStartY + layoutStepY * i + jy, layoutStartZ + layoutStepZ * i + jz };
                particles[i].velocity = { randVelRange * dist(randomEngine), randVelRange * dist(randomEngine), randVelRange * randVelZScale * dist(randomEngine) };
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            play = false;
            for (uint32_t i = 0; i < kNumInstance; ++i) {
                particles[i].transform.scale = { spriteScaleXY, spriteScaleXY, 1.0f };
                particles[i].transform.rotate = { 0.0f,0.0f,0.0f };
                particles[i].transform.translate = { layoutStartX + layoutStepX * i, layoutStartY + layoutStepY * i, layoutStartZ + layoutStepZ * i };
                particles[i].velocity = { 0.0f,0.0f,0.0f };
            }
        }

        // Lighting / Material
        ImGui::SeparatorText("Lighting / Material");
        // Object3dからライトデータを取得していじる
        auto* lightData = object3d->GetDirectionalLightData();
        auto* matData = modelFence->GetMaterialData(); // モデルからマテリアル取得

        ImGui::Text("Lighting Direction");
        ImGui::SliderFloat("lx", &lightData->direction.x, -10.0f, 10.0f);
        ImGui::SliderFloat("ly", &lightData->direction.y, -10.0f, 10.0f);
        ImGui::SliderFloat("lz", &lightData->direction.z, -10.0f, 10.0f);
        lightData->direction = Normalize(lightData->direction);

        ImGui::Text("Color");
        if (matData) {
            ImGui::ColorEdit4("Color", &matData->color.x);
        }

        // UV Transform (Particle用)
        ImGui::SeparatorText("UV Transform (Particle)");
        ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
        ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
        ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);

        // UV行列計算
        Vector3 uvScale = { uvTransformSprite.scale.x, uvTransformSprite.scale.y, 1.0f };
        Vector3 uvRotate = { 0.0f, 0.0f, uvTransformSprite.rotate.z };
        Vector3 uvTrans = { uvTransformSprite.translate.x, uvTransformSprite.translate.y, 0.0f };
        materialDataSprite->uvTransform = MakeAffine(uvScale, uvRotate, uvTrans);

        // Blend Mode
        ImGui::SeparatorText("Material Blend Mode");
        ImGui::Combo("BlendMode", &currentBlendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames));

        ImGui::End();

        // ==== 更新処理 ====
        object3d->Update();

        // パーティクル更新
        if (play) {
            const float kDeltaTime = 1.0f / 60.0f;
            for (uint32_t i = 0; i < kNumInstance; ++i) {
                particles[i].transform.translate.x += particles[i].velocity.x * kDeltaTime;
                particles[i].transform.translate.y += particles[i].velocity.y * kDeltaTime;
                particles[i].transform.translate.z += particles[i].velocity.z * kDeltaTime;
            }
        }

        Matrix4x4 cameraMatrix = MakeAffine(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
        Matrix4x4 viewMatrix = Inverse(cameraMatrix);
        Matrix4x4 projectionMatrix = PerspectiveFov(0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 100.0f);

        for (uint32_t i = 0; i < kNumInstance; ++i) {
            Matrix4x4 world = MakeAffine(particles[i].transform.scale, particles[i].transform.rotate, particles[i].transform.translate);
            Matrix4x4 wvp = Multipty(world, Multipty(viewMatrix, projectionMatrix));
            instancingData[i].World = world;
            instancingData[i].WVP = wvp;
        }

        // ==== 描画 ====
        ImGui::Render();

        dxCommon->PreDraw();

        ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap };
        commandList->SetDescriptorHeaps(1, heaps);

        // --- 3Dオブジェクト描画 ---
        // ブレンドモードを指定して共通設定を呼び出し
        object3dCommon->CommonDrawSetting((Object3dCommon::BlendMode)currentBlendMode);

        object3d->Draw();

        // --- パーティクル描画 ---
        spriteCommon->CommonDrawSetting();

        commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
        commandList->IASetIndexBuffer(&indexBufferViewSprite);
        commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(1, instancingSrvGPU);
        commandList->SetGraphicsRootDescriptorTable(2, texManager->GetSrvHandleGPU(texIndexUvChecker));
        commandList->DrawIndexedInstanced(6, kNumInstance, 0, 0, 0);

        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

        dxCommon->PostDraw();
    }

    // ==== 終了処理 ====
    texManager->ReleaseIntermediateResources();
    vertexResourceSprite->Release();
    indexResourceSprite->Release();
    materialResourceSprite->Release();
    instancingResource->Release();

    texManager->Finalize();
    modelManager->Finalize();

    delete object3d;
    delete object3dCommon;
    delete spriteCommon;
    delete dxCommon;
    delete winApp;

    CoUninitialize();

    return 0;
}