#include "MyGame.h"
#include <cmath>
#include <random>

// デバッグ時のみImGuiのヘッダをインクルード
#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif

MyGame::MyGame() {}
MyGame::~MyGame() {}

void MyGame::Initialize() {
    // --- 基盤初期化 ---
    winApp_ = new WinApp();
    winApp_->Initialize();

    dxCommon_ = new DirectXCommon();
    dxCommon_->Initialize(winApp_);

    srvManager_ = SrvManager::GetInstance();
    srvManager_->Initialize(dxCommon_);

    // ImGuiManager初期化
    imguiManager_ = new ImGuiManager();
    imguiManager_->Initialize(winApp_, dxCommon_);

    texManager_ = TextureManager::GetInstance();
    texManager_->Initialize(dxCommon_, srvManager_);

    spriteCommon_ = new SpriteCommon();
    spriteCommon_->Initialize(dxCommon_);

    object3dCommon_ = new Object3dCommon();
    object3dCommon_->Initialize(dxCommon_);

    modelManager_ = ModelManager::GetInstance();
    modelManager_->Initialize(dxCommon_);

    input_ = new Input();
    input_->Initialize(winApp_);

    particleManager_ = ParticleManager::GetInstance();
    particleManager_->Initialize(dxCommon_, srvManager_);

    audio_ = Audio::GetInstance();
    audio_->Initialize();

    // --- モデルロード ---
    modelManager_->LoadModel("resources/obj/fence/fence.obj");
    modelFence_ = modelManager_->FindModel("resources/obj/fence/fence.obj");

    // --- カメラ生成 ---
    camera_ = new Camera();
    camera_->SetTranslate({ 0.0f, 4.0f, -10.0f });
    camera_->SetRotate({ 0.3f, 0.0f, 0.0f });

    // --- BGMやSEの読み込み ---
    audio_->LoadAudio("resources/sounds/Alarm01.mp3");

    // --- 3Dオブジェクト生成 ---
    object3d_ = new Object3d();
    object3d_->Initialize(object3dCommon_);
    object3d_->SetModel(modelFence_);
    object3d_->SetTranslate({ 0.0f, 0.0f, 0.0f });
    object3d_->SetCamera(camera_);

    // テクスチャロード
    texManager_->LoadTexture("resources/obj/fence/fence.png");
    texManager_->LoadTexture("resources/obj/axis/uvChecker.png");

    texIndexUvChecker_ = texManager_->GetTextureIndexByFilePath("resources/obj/axis/uvChecker.png");
    texIndexFence_ = texManager_->GetTextureIndexByFilePath("resources/obj/fence/fence.png");
}

void MyGame::Finalize() {
    // --- 終了処理 ---
    texManager_->ReleaseIntermediateResources();
    particleManager_->Finalize();
    imguiManager_->Finalize(); // ImGui解放
    texManager_->Finalize();
    modelManager_->Finalize();

    delete imguiManager_; // 解放
    delete object3d_;
    delete camera_;
    delete object3dCommon_;
    delete spriteCommon_;
    delete dxCommon_;
    delete winApp_;
    delete input_;

    audio_->Finalize();
}

void MyGame::Run() {
    // メインループ
    while (true) {
        if (winApp_->ProcessMessage()) break;

        Update();
        Draw();
    }
}

void MyGame::Update() {
    // ImGui受付開始
    imguiManager_->Begin();

    input_->Update();

    // --- カメラ操作 ---
#ifdef _DEBUG
    // ImGuiがマウスをキャプチャしている時はカメラを動かさない
    if (!ImGui::GetIO().WantCaptureMouse && input_->MouseDown(Input::MouseLeft)) {
#else
    if (input_->MouseDown(Input::MouseLeft)) {
#endif
        Vector3 rot = camera_->GetRotate();
        rot.y += input_->MouseDeltaX() * 0.0025f;
        rot.x += input_->MouseDeltaY() * 0.0025f;
        camera_->SetRotate(rot);
    }

    Vector3 move = { 0,0,0 };
    if (input_->PushKey(DIK_W)) move.z += 1.0f;
    if (input_->PushKey(DIK_S)) move.z -= 1.0f;
    if (input_->PushKey(DIK_D)) move.x += 1.0f;
    if (input_->PushKey(DIK_A)) move.x -= 1.0f;

    if (move.x != 0 || move.z != 0) {
        float speed = 0.1f;
        Vector3 rot = camera_->GetRotate();
        Vector3 trans = camera_->GetTranslate();
        trans.x += (move.x * std::cos(rot.y) + move.z * std::sin(rot.y)) * speed;
        trans.z += (move.x * -std::sin(rot.y) + move.z * std::cos(rot.y)) * speed;
        camera_->SetTranslate(trans);
    }

    if (input_->PushKey(DIK_0)) {
        audio_->PlayAudio("resources/sounds/Alarm01.mp3");
    }

    // ★更新
    camera_->Update();
    object3d_->Update();

    // --- パーティクル操作 ---
    if (input_->PushKey(DIK_SPACE)) {
        particleManager_->Emit("resources/obj/axis/uvChecker.png", { 0,0,0 }, 2);
    }
#ifdef _DEBUG
    if (!ImGui::GetIO().WantCaptureMouse && input_->MouseTrigger(Input::MouseLeft)) {
#else
    if (input_->MouseTrigger(Input::MouseLeft)) {
#endif
        Vector3 pos = camera_->GetTranslate();
        Vector3 rot = camera_->GetRotate();
        pos.x += std::sin(rot.y) * 5.0f;
        pos.z += std::cos(rot.y) * 5.0f;
        particleManager_->Emit("resources/obj/axis/uvChecker.png", pos, 10);
    }

    particleManager_->Update(camera_);

    // --- ImGui (デバッグ時のみ) ---
#ifdef _DEBUG
    ImGui::Begin("Debug Menu");
    ImGui::SeparatorText("Camera");
    Vector3 camTrans = camera_->GetTranslate();
    if (ImGui::DragFloat3("Cam Pos", &camTrans.x, 0.1f)) camera_->SetTranslate(camTrans);

    ImGui::SeparatorText("Model Transform");
    Transform& tf = object3d_->GetTransform();
    ImGui::DragFloat3("Pos", &tf.translate.x, 0.1f);
    ImGui::DragFloat3("Rot", &tf.rotate.x, 0.01f);
    ImGui::DragFloat3("Scl", &tf.scale.x, 0.1f);

    ImGui::SeparatorText("Model Texture");
    const char* modelTextureNames[] = { "uvChecker", "FenceTexture" };
    if (ImGui::Combo("Texture", &currentModelTexture_, modelTextureNames, IM_ARRAYSIZE(modelTextureNames))) {
        if (currentModelTexture_ == 0) modelFence_->SetTextureIndex(texIndexUvChecker_);
        else modelFence_->SetTextureIndex(texIndexFence_);
    }

    ImGui::SeparatorText("Lighting");
    auto* lightData = object3d_->GetDirectionalLightData();
    if (lightData) {
        ImGui::SliderFloat3("LightDir", &lightData->direction.x, -1.0f, 1.0f);
        ImGui::ColorEdit4("LightColor", &lightData->color.x);
    }

    ImGui::SeparatorText("Blend Mode");
    ImGui::Combo("Blend", &currentBlendMode_, blendModeNames_, IM_ARRAYSIZE(blendModeNames_));

    ImGui::SeparatorText("Particle Demo");
    if (ImGui::Button("Emit Particles")) {
        for (int i = 0; i < 5; ++i) {
            Vector3 pos = {
                layoutStartX_ + layoutStepX_ * i,
                layoutStartY_ + layoutStepY_ * i,
                layoutStartZ_ + layoutStepZ_ * i
            };
            particleManager_->Emit("resources/obj/axis/uvChecker.png", pos, 2);
        }
    }
    ImGui::End();

    // --- ImGuiのウィンドウ作成 (Sprite Viewer) ---
    ImGui::Begin("Sprite Viewer");

    // 1. 表示したいテクスチャのパスを指定し、インデックスを取得
    std::string texPath = "resources/obj/axis/uvChecker.png";
    texManager_->LoadTexture(texPath);
    uint32_t texIndex = texManager_->GetTextureIndexByFilePath(texPath);

    // 2. TextureManagerからGPUハンドルを取得
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = texManager_->GetSrvHandleGPU(texIndex);

    // 3. テクスチャのサイズを取得
    const DirectX::TexMetadata& metadata = texManager_->GetMetaData(texIndex);
    ImVec2 imageSize(static_cast<float>(metadata.width), static_cast<float>(metadata.height));

    //  4. ImGui::Image で描画！
    ImGui::Image(reinterpret_cast<ImTextureID>(gpuHandle.ptr), imageSize);

    ImGui::End();
#endif

    // ImGui受付終了
    imguiManager_->End();
    }

void MyGame::Draw() {
    // --- 描画 ---
    dxCommon_->PreDraw();
    srvManager_->PreDraw();

    object3dCommon_->CommonDrawSetting((Object3dCommon::BlendMode)currentBlendMode_);
    object3d_->Draw();

    particleManager_->Draw();

    // ImGui描画
    imguiManager_->Draw();

    dxCommon_->PostDraw();
}