#define _USE_MATH_DEFINES
#include <Windows.h>
#include "WinApp.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "SpriteCommon.h"
#include "Sprite.h"
#include "Object3dCommon.h"
#include "Object3d.h"
#include "ModelManager.h"
#include "Model.h"
#include "Camera.h"
#include "ParticleManager.h"
#include "Input.h"
#include "ImGuiManager.h" // 追加
#include <cmath>
#include <random>

// デバッグ時のみImGuiのヘッダをインクルード
#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
    return EXCEPTION_EXECUTE_HANDLER;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    CoInitializeEx(0, COINIT_MULTITHREADED);
    SetUnhandledExceptionFilter(ExportDump);

    // --- 基盤初期化 ---
    WinApp* winApp = new WinApp();
    winApp->Initialize();

    DirectXCommon* dxCommon = new DirectXCommon();
    dxCommon->Initialize(winApp);

    SrvManager* srvManager = SrvManager::GetInstance();
    srvManager->Initialize(dxCommon);

    // ImGuiManager初期化
    ImGuiManager* imguiManager = new ImGuiManager();
    imguiManager->Initialize(winApp, dxCommon);

    TextureManager* texManager = TextureManager::GetInstance();
    texManager->Initialize(dxCommon, srvManager);

    SpriteCommon* spriteCommon = new SpriteCommon();
    spriteCommon->Initialize(dxCommon);

    Object3dCommon* object3dCommon = new Object3dCommon();
    object3dCommon->Initialize(dxCommon);

    ModelManager* modelManager = ModelManager::GetInstance();
    modelManager->Initialize(dxCommon);

    Input input;
    input.Initialize(winApp);

    ParticleManager* particleManager = ParticleManager::GetInstance();
    particleManager->Initialize(dxCommon, srvManager);

    // --- モデルロード ---
    modelManager->LoadModel("resources/obj/fence/fence.obj");
    Model* modelFence = modelManager->FindModel("resources/obj/fence/fence.obj");

    // --- カメラ生成 ---
    Camera* camera = new Camera();
    camera->SetTranslate({ 0.0f, 4.0f, -10.0f });
    camera->SetRotate({ 0.3f, 0.0f, 0.0f });

    // --- 3Dオブジェクト生成 ---
    Object3d* object3d = new Object3d();
    object3d->Initialize(object3dCommon);
    object3d->SetModel(modelFence);
    object3d->SetTranslate({ 0.0f, 0.0f, 0.0f });
    object3d->SetCamera(camera);

    // テクスチャロード
    texManager->LoadTexture("resources/obj/fence/fence.png");
    texManager->LoadTexture("resources/obj/axis/uvChecker.png");

    uint32_t texIndexUvChecker = texManager->GetTextureIndexByFilePath("resources/obj/axis/uvChecker.png");
    uint32_t texIndexFence = texManager->GetTextureIndexByFilePath("resources/obj/fence/fence.png");

    // --- 変数 ---
    int currentModelTexture = 1; // 0:uvChecker, 1:Fence
    int currentBlendMode = 0;
#ifdef _DEBUG
    const char* blendModeNames[] = { "Normal","Add","Subtract","Multiply","Screen","None" };
#endif

    float layoutStartX = -1.4f;
    float layoutStartY = -0.8f;
    float layoutStartZ = 0.0f;
    float layoutStepX = 0.22f;
    float layoutStepY = 0.11f;
    float layoutStepZ = 0.05f;

    // メインループ
    while (true) {
        if (winApp->ProcessMessage()) break;

        // ImGui受付開始
        imguiManager->Begin();

        input.Update();

        // --- カメラ操作 ---
#ifdef _DEBUG
        // ImGuiがマウスをキャプチャしている時はカメラを動かさない
        if (!ImGui::GetIO().WantCaptureMouse && input.MouseDown(Input::MouseLeft)) {
#else
        if (input.MouseDown(Input::MouseLeft)) {
#endif
            Vector3 rot = camera->GetRotate();
            rot.y += input.MouseDeltaX() * 0.0025f;
            rot.x += input.MouseDeltaY() * 0.0025f;
            camera->SetRotate(rot);
        }

        Vector3 move = { 0,0,0 };
        if (input.PushKey(DIK_W)) move.z += 1.0f;
        if (input.PushKey(DIK_S)) move.z -= 1.0f;
        if (input.PushKey(DIK_D)) move.x += 1.0f;
        if (input.PushKey(DIK_A)) move.x -= 1.0f;

        if (move.x != 0 || move.z != 0) {
            float speed = 0.1f;
            Vector3 rot = camera->GetRotate();
            Vector3 trans = camera->GetTranslate();
            trans.x += (move.x * std::cos(rot.y) + move.z * std::sin(rot.y)) * speed;
            trans.z += (move.x * -std::sin(rot.y) + move.z * std::cos(rot.y)) * speed;
            camera->SetTranslate(trans);
        }

        // ★更新
        camera->Update();
        object3d->Update();

        // --- パーティクル操作 ---
        if (input.PushKey(DIK_SPACE)) {
            particleManager->Emit("resources/obj/axis/uvChecker.png", { 0,0,0 }, 2);
        }
#ifdef _DEBUG
        if (!ImGui::GetIO().WantCaptureMouse && input.MouseTrigger(Input::MouseLeft)) {
#else
        if (input.MouseTrigger(Input::MouseLeft)) {
#endif
            Vector3 pos = camera->GetTranslate();
            Vector3 rot = camera->GetRotate();
            pos.x += std::sin(rot.y) * 5.0f;
            pos.z += std::cos(rot.y) * 5.0f;
            particleManager->Emit("resources/obj/axis/uvChecker.png", pos, 10);
        }

        particleManager->Update(camera);

        // --- ImGui (デバッグ時のみ) ---
#ifdef _DEBUG
        ImGui::Begin("Debug Menu");
        ImGui::SeparatorText("Camera");
        Vector3 camTrans = camera->GetTranslate();
        if (ImGui::DragFloat3("Cam Pos", &camTrans.x, 0.1f)) camera->SetTranslate(camTrans);

        ImGui::SeparatorText("Model Transform");
        Transform& tf = object3d->GetTransform();
        ImGui::DragFloat3("Pos", &tf.translate.x, 0.1f);
        ImGui::DragFloat3("Rot", &tf.rotate.x, 0.01f);
        ImGui::DragFloat3("Scl", &tf.scale.x, 0.1f);

        ImGui::SeparatorText("Model Texture");
        const char* modelTextureNames[] = { "uvChecker", "FenceTexture" };
        if (ImGui::Combo("Texture", &currentModelTexture, modelTextureNames, IM_ARRAYSIZE(modelTextureNames))) {
            if (currentModelTexture == 0) modelFence->SetTextureIndex(texIndexUvChecker);
            else modelFence->SetTextureIndex(texIndexFence);
        }

        ImGui::SeparatorText("Lighting");
        auto* lightData = object3d->GetDirectionalLightData();
        if (lightData) {
            ImGui::SliderFloat3("LightDir", &lightData->direction.x, -1.0f, 1.0f);
            ImGui::ColorEdit4("LightColor", &lightData->color.x);
        }

        ImGui::SeparatorText("Blend Mode");
        ImGui::Combo("Blend", &currentBlendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames));

        ImGui::SeparatorText("Particle Demo");
        if (ImGui::Button("Emit Particles")) {
            for (int i = 0; i < 5; ++i) {
                Vector3 pos = {
                    layoutStartX + layoutStepX * i,
                    layoutStartY + layoutStepY * i,
                    layoutStartZ + layoutStepZ * i
                };
                particleManager->Emit("resources/obj/axis/uvChecker.png", pos, 2);
            }
        }
        ImGui::End();
#endif

        // ImGui受付終了
        imguiManager->End();

        // --- 描画 ---
        dxCommon->PreDraw();
        srvManager->PreDraw();

        object3dCommon->CommonDrawSetting((Object3dCommon::BlendMode)currentBlendMode);
        object3d->Draw();

        particleManager->Draw();

        // ImGui描画
        imguiManager->Draw();

        dxCommon->PostDraw();
        }

    texManager->ReleaseIntermediateResources();
    particleManager->Finalize();
    imguiManager->Finalize(); // ImGui解放
    texManager->Finalize();
    modelManager->Finalize();

    delete imguiManager; // 解放
    delete object3d;
    delete camera;
    delete object3dCommon;
    delete spriteCommon;
    delete dxCommon;
    delete winApp;

    CoUninitialize();
    return 0;
        }