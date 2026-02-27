#include "GameScene.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "MyGame.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "Audio.h"
#include <cmath>

#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif

void GameScene::Initialize() {
    auto modelManager = ModelManager::GetInstance();
    auto texManager = TextureManager::GetInstance();
    auto object3dCommon = MyGame::GetInstance()->GetObject3dCommon();
    auto spriteCommon = MyGame::GetInstance()->GetSpriteCommon();

    // --- モデルロード ---
    modelFence_ = modelManager->FindModel("resources/obj/fence/fence.obj");

    // --- カメラ生成 ---
    camera_ = new Camera();
    camera_->SetTranslate({ 0.0f, 4.0f, -10.0f });
    camera_->SetRotate({ 0.3f, 0.0f, 0.0f });

    // --- 3Dオブジェクト生成 ---
    object3d_ = new Object3d();
    object3d_->Initialize(object3dCommon);
    object3d_->SetModel(modelFence_);
    object3d_->SetTranslate({ 0.0f, 0.0f, 0.0f });
    object3d_->SetCamera(camera_);

    // テクスチャインデックス取得
    texIndexUvChecker_ = texManager->GetTextureIndexByFilePath("resources/obj/axis/uvChecker.png");
    texIndexFence_ = texManager->GetTextureIndexByFilePath("resources/obj/fence/fence.png");

    // デバッグ対象用スプライトの初期化
    debugSprite_ = new Sprite();
    debugSprite_->Initialize(spriteCommon);
    debugSprite_->SetTexture("resources/obj/axis/uvChecker.png");
    debugSprite_->SetPosition({ 100.0f, 100.0f });
    debugSprite_->SetSize({ 100.0f, 100.0f });
}

void GameScene::Finalize() {
    delete debugSprite_;
    delete object3d_;
    delete camera_;
}

void GameScene::Update() {
    auto input = MyGame::GetInstance()->GetInput();
    auto particleManager = ParticleManager::GetInstance();
    auto audio = Audio::GetInstance();
    auto texManager = TextureManager::GetInstance();

    // [T]キーでタイトルに戻る処理
    if (input->PushKey(DIK_T)) {
        SceneManager::GetInstance()->ChangeScene(new TitleScene());
        return;
    }

    // --- カメラ操作 ---
#ifdef _DEBUG
    if (!ImGui::GetIO().WantCaptureMouse && input->MouseDown(Input::MouseLeft)) {
#else
    if (input->MouseDown(Input::MouseLeft)) {
#endif
        Vector3 rot = camera_->GetRotate();
        rot.y += input->MouseDeltaX() * 0.0025f;
        rot.x += input->MouseDeltaY() * 0.0025f;
        camera_->SetRotate(rot);
    }

    Vector3 move = { 0,0,0 };
    if (input->PushKey(DIK_W)) move.z += 1.0f;
    if (input->PushKey(DIK_S)) move.z -= 1.0f;
    if (input->PushKey(DIK_D)) move.x += 1.0f;
    if (input->PushKey(DIK_A)) move.x -= 1.0f;

    if (move.x != 0 || move.z != 0) {
        float speed = 0.1f;
        Vector3 rot = camera_->GetRotate();
        Vector3 trans = camera_->GetTranslate();
        trans.x += (move.x * std::cos(rot.y) + move.z * std::sin(rot.y)) * speed;
        trans.z += (move.x * -std::sin(rot.y) + move.z * std::cos(rot.y)) * speed;
        camera_->SetTranslate(trans);
    }

    if (input->PushKey(DIK_0)) {
        audio->PlayAudio("resources/sounds/Alarm01.mp3");
    }

    // 各種更新
    camera_->Update();
    object3d_->Update();
    debugSprite_->Update();

    // --- パーティクル操作 ---
    if (input->PushKey(DIK_SPACE)) {
        particleManager->Emit("resources/obj/axis/uvChecker.png", { 0,0,0 }, 2);
    }
#ifdef _DEBUG
    if (!ImGui::GetIO().WantCaptureMouse && input->MouseTrigger(Input::MouseLeft)) {
#else
    if (input->MouseTrigger(Input::MouseLeft)) {
#endif
        Vector3 pos = camera_->GetTranslate();
        Vector3 rot = camera_->GetRotate();
        pos.x += std::sin(rot.y) * 5.0f;
        pos.z += std::cos(rot.y) * 5.0f;
        particleManager->Emit("resources/obj/axis/uvChecker.png", pos, 10);
    }

    particleManager->Update(camera_);

    // ==========================================
    // ImGui メニュー群 (デバッグ時のみ)
    // ==========================================
#ifdef _DEBUG

    // ------------------------------------------
    // スプライトのデバッグテキスト
    // ------------------------------------------
    // ※ ウィンドウサイズ (500, 200) はスライドの指示通りの数値に合わせてください
    ImGui::SetNextWindowSize(ImVec2(500, 200), ImGuiCond_Once);
    ImGui::Begin("DebugText"); // ※ ウィンドウ名もスライドの指定に合わせてください
    Vector2 spritePos = debugSprite_->GetPosition();
    // 整数4桁・小数1桁 (%4.1f) の書式指定
    ImGui::DragFloat2("Sprite Pos", &spritePos.x, 1.0f, -9999.0f, 9999.0f, "%4.1f");
    debugSprite_->SetPosition(spritePos);
    ImGui::End();

    ImGui::Begin("Game Scene Menu");
    ImGui::Text("Press [T] to return to Title");

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
            particleManager->Emit("resources/obj/axis/uvChecker.png", pos, 2);
        }
    }
    ImGui::End();
    ImGui::Begin("Sprite Viewer");
    std::string texPath = "resources/obj/axis/uvChecker.png";
    uint32_t texIndex = texManager->GetTextureIndexByFilePath(texPath);
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = texManager->GetSrvHandleGPU(texIndex);
    const DirectX::TexMetadata& metadata = texManager->GetMetaData(texIndex);
    ImVec2 imageSize(static_cast<float>(metadata.width), static_cast<float>(metadata.height));
    ImGui::Image(reinterpret_cast<ImTextureID>(gpuHandle.ptr), imageSize);
    ImGui::End();

#endif
    }

void GameScene::Draw() {
    auto object3dCommon = MyGame::GetInstance()->GetObject3dCommon();
    auto particleManager = ParticleManager::GetInstance();
    auto spriteCommon = MyGame::GetInstance()->GetSpriteCommon();

    // 3Dオブジェクトとパーティクルの描画
    object3dCommon->CommonDrawSetting((Object3dCommon::BlendMode)currentBlendMode_);
    object3d_->Draw();
    particleManager->Draw();

    // スプライトの描画 (3Dの後、ImGuiの前に描画します)
    spriteCommon->CommonDrawSetting();
    debugSprite_->Draw();
}