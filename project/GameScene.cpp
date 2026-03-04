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

    // --- モデル取得・生成 ---
    modelFence_ = modelManager->FindModel("resources/obj/fence/fence.obj");

    modelSphere_ = modelManager->CreateSphere("InternalSphere", 16);

    camera_ = std::make_unique<Camera>();
    camera_->SetTranslate({ 0.0f, 2.0f, -10.0f });
    camera_->SetRotate({ 0.1f, 0.0f, 0.0f });

    object3d_ = std::make_unique<Object3d>();
    object3d_->Initialize(object3dCommon);
    object3d_->SetModel(modelFence_);
    object3d_->SetTranslate({ -2.0f, 0.0f, 0.0f });
    object3d_->SetCamera(camera_.get());

    object3dSphere_ = std::make_unique<Object3d>();
    object3dSphere_->Initialize(object3dCommon);
    object3dSphere_->SetModel(modelSphere_); // 生成した球体をセット
    object3dSphere_->SetTranslate({ 2.0f, 0.0f, 0.0f });
    object3dSphere_->SetCamera(camera_.get());

    texManager->LoadTexture("resources/obj/axis/uvChecker.png");
    texManager->LoadTexture("resources/obj/fence/fence.png");
    texManager->LoadTexture("resources/obj/monsterBall/monsterBall.png");

    texIndexUvChecker_ = texManager->GetTextureIndexByFilePath("resources/obj/axis/uvChecker.png");
    texIndexFence_ = texManager->GetTextureIndexByFilePath("resources/obj/fence/fence.png");
    texIndexMonsterBall_ = texManager->GetTextureIndexByFilePath("resources/obj/monsterBall/monsterBall.png");

    if (modelSphere_) {
        // 初期テクスチャをモンスターボールに設定
        modelSphere_->SetTextureIndex(texIndexMonsterBall_);
    }

    debugSprite_ = std::make_unique<Sprite>();
    debugSprite_->Initialize(spriteCommon);
    debugSprite_->SetTexture("resources/obj/axis/uvChecker.png");
    debugSprite_->SetPosition({ 100.0f, 100.0f });
    debugSprite_->SetSize({ 100.0f, 100.0f });
}

void GameScene::Finalize() {}

void GameScene::Update() {
    auto input = MyGame::GetInstance()->GetInput();
    auto particleManager = ParticleManager::GetInstance();
    auto audio = Audio::GetInstance();
    auto texManager = TextureManager::GetInstance();

    if (input->PushKey(DIK_T)) {
        SceneManager::GetInstance()->ChangeScene(std::make_unique<TitleScene>());
        return;
    }

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

    if (input->PushKey(DIK_0)) audio->PlayAudio("resources/sounds/Alarm01.mp3");

    camera_->Update();
    object3d_->Update();
    object3dSphere_->Update();
    debugSprite_->Update();

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

    particleManager->Update(camera_.get());

#ifdef _DEBUG
    ImGui::SetNextWindowSize(ImVec2(500, 200), ImGuiCond_Once);
    ImGui::Begin("DebugText");
    Vector2 spritePos = debugSprite_->GetPosition();
    ImGui::DragFloat2("Sprite Pos", &spritePos.x, 1.0f, -9999.0f, 9999.0f, "%4.1f");
    debugSprite_->SetPosition(spritePos);
    ImGui::End();

    ImGui::Begin("Game Scene Menu");
    ImGui::Text("Press [T] to return to Title");
    ImGui::SeparatorText("Camera");
    Vector3 camTrans = camera_->GetTranslate();
    if (ImGui::DragFloat3("Cam Pos", &camTrans.x, 0.1f)) camera_->SetTranslate(camTrans);

    ImGui::SeparatorText("Target Object Selection");
    ImGui::Combo("Target", &targetObjectIndex_, "Fence\0Sphere\0");

    Object3d* targetObj = (targetObjectIndex_ == 0) ? object3d_.get() : object3dSphere_.get();

    ImGui::SeparatorText("Model Transform");
    Transform& tf = targetObj->GetTransform();
    ImGui::DragFloat3("Pos", &tf.translate.x, 0.1f);
    ImGui::DragFloat3("Rot", &tf.rotate.x, 0.01f);
    ImGui::DragFloat3("Scl", &tf.scale.x, 0.1f);

    ImGui::SeparatorText("Model Texture");
    const char* modelTextureNames[] = { "uvChecker", "FenceTexture", "MonsterBall" };
    if (ImGui::Combo("Texture", &currentModelTexture_, modelTextureNames, IM_ARRAYSIZE(modelTextureNames))) {
        Model* targetModel = (targetObjectIndex_ == 0) ? modelFence_ : modelSphere_;
        if (targetModel) {
            if (currentModelTexture_ == 0) targetModel->SetTextureIndex(texIndexUvChecker_);
            else if (currentModelTexture_ == 1) targetModel->SetTextureIndex(texIndexFence_);
            else if (currentModelTexture_ == 2) targetModel->SetTextureIndex(texIndexMonsterBall_);
        }
    }

    ImGui::SeparatorText("Lighting & Material");
    auto* lightData = targetObj->GetDirectionalLightData();
    if (lightData) {
        if (ImGui::SliderFloat3("LightDir", &lightData->direction.x, -1.0f, 1.0f)) {
            float len = std::sqrt(lightData->direction.x * lightData->direction.x +
                lightData->direction.y * lightData->direction.y +
                lightData->direction.z * lightData->direction.z);
            if (len > 0.0f) {
                lightData->direction.x /= len;
                lightData->direction.y /= len;
                lightData->direction.z /= len;
            }
        }
        ImGui::ColorEdit3("LightColor", &lightData->color.x);
        ImGui::DragFloat("Intensity", &lightData->intensity, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Shininess", &lightData->shininess, 1.0f, 1.0f, 256.0f, "%.1f");
    }

    ImGui::SeparatorText("Blend Mode");
    ImGui::Combo("Blend", &currentBlendMode_, blendModeNames_, IM_ARRAYSIZE(blendModeNames_));
    ImGui::End();
#endif
    }

void GameScene::Draw() {
    auto object3dCommon = MyGame::GetInstance()->GetObject3dCommon();
    auto particleManager = ParticleManager::GetInstance();
    auto spriteCommon = MyGame::GetInstance()->GetSpriteCommon();

    object3dCommon->CommonDrawSetting((Object3dCommon::BlendMode)currentBlendMode_);

    object3d_->Draw();
    object3dSphere_->Draw();

    particleManager->Draw();

    spriteCommon->CommonDrawSetting();
    debugSprite_->Draw();
}