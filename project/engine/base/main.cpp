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
#include "Sphere.h" // ★必須: Sphereヘッダ
#include "Input.h"
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include <cmath>
#include <random>

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

    // --- モデルロード (Fence) ---
    modelManager->LoadModel("resources/obj/fence/fence.obj");
    Model* modelFence = modelManager->FindModel("resources/obj/fence/fence.obj");

    // --- スフィア生成 (★ここが消えていました) ---
    Sphere* sphere = new Sphere();
    sphere->Initialize(dxCommon);
    // sphere->SetTexture("resources/monsterBall.png"); // 必要ならテクスチャ設定

    // --- カメラ生成 ---
    Camera* camera = new Camera();
    camera->SetTranslate({ 0.0f, 2.0f, -10.0f });
    camera->SetRotate({ 0.1f, 0.0f, 0.0f });

    // --- 3Dオブジェクト生成 (Fence用) ---
    Object3d* object3dFence = new Object3d();
    object3dFence->Initialize(object3dCommon);
    object3dFence->SetModel(modelFence);
    object3dFence->SetTranslate({ -2.0f, 0.0f, 0.0f });
    object3dFence->SetCamera(camera);

    // --- 3Dオブジェクト生成 (Sphere用) (★追加) ---
    Object3d* object3dSphere = new Object3d();
    object3dSphere->Initialize(object3dCommon);
    object3dSphere->SetSphere(sphere);
    object3dSphere->SetTranslate({ 2.0f, 0.0f, 0.0f });
    object3dSphere->SetCamera(camera);

    // テクスチャロード
    texManager->LoadTexture("resources/obj/fence/fence.png");
    texManager->LoadTexture("resources/obj/axis/uvChecker.png");

    uint32_t texIndexUvChecker = texManager->GetTextureIndexByFilePath("resources/obj/axis/uvChecker.png");
    uint32_t texIndexFence = texManager->GetTextureIndexByFilePath("resources/obj/fence/fence.png");

    // --- 変数 (ImGui用) ---
    int currentModelTexture = 1;
    int currentBlendMode = 0;
    const char* blendModeNames[] = { "Normal","Add","Subtract","Multiply","Screen","None" };
    // ライティング種類 (Toon含む)
    const char* lightingTypeNames[] = { "Half-Lambert", "Lambert", "Phong", "Blinn-Phong", "Toon" };

    // メインループ
    while (true) {
        if (winApp->ProcessMessage()) break;

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        input.Update();

        // --- カメラ操作 ---
        if (!ImGui::GetIO().WantCaptureMouse && input.MouseDown(Input::MouseLeft)) {
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

        // 更新
        camera->Update();
        object3dFence->Update();
        object3dSphere->Update(); // ★更新忘れずに

        // --- パーティクル操作 ---
        if (input.PushKey(DIK_SPACE)) {
            particleManager->Emit("resources/obj/axis/uvChecker.png", { 0,0,0 }, 2);
        }
        if (!ImGui::GetIO().WantCaptureMouse && input.MouseTrigger(Input::MouseLeft)) {
            // 左クリックでパーティクル
            // (カメラ操作と被る場合はキーボード等に変更してください)
        }
        particleManager->Update(camera);

        // --- ImGui ---
        ImGui::Begin("Settings");

        if (ImGui::CollapsingHeader("Sphere Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            Transform& tf = object3dSphere->GetTransform();
            ImGui::DragFloat3("Trans", &tf.translate.x, 0.1f);

            Sphere::Material* material = sphere->GetMaterialData();
            if (material) {
                // ライティングタイプ選択
                ImGui::Combo("Lighting", &material->lightingType, lightingTypeNames, IM_ARRAYSIZE(lightingTypeNames));
                // 光沢度 (Toonのときは点の大きさに影響)
                ImGui::DragFloat("Shininess", &material->shininess, 1.0f, 1.0f, 1000.0f);
            }
        }

        if (ImGui::CollapsingHeader("Light Direction")) {
            auto* lightData = object3dSphere->GetDirectionalLightData();
            if (lightData) {
                ImGui::SliderFloat3("Dir", &lightData->direction.x, -1.0f, 1.0f);
            }
        }

        ImGui::End();

        // --- 描画 ---
        ImGui::Render();
        dxCommon->PreDraw();
        srvManager->PreDraw();

        object3dCommon->CommonDrawSetting((Object3dCommon::BlendMode)currentBlendMode);

        object3dFence->Draw();  // フェンス
        object3dSphere->Draw(); // ★スフィア

        particleManager->Draw();

        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList());
        dxCommon->PostDraw();
    }

    texManager->ReleaseIntermediateResources();
    particleManager->Finalize();
    texManager->Finalize();
    modelManager->Finalize();

    delete object3dFence;
    delete object3dSphere; // ★削除忘れずに
    delete sphere;         // ★削除忘れずに
    delete camera;
    delete object3dCommon;
    delete spriteCommon;
    delete dxCommon;
    delete winApp;

    CoUninitialize();
    return 0;
}