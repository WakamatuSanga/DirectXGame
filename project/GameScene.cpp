#include "GameScene.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "MyGame.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "Audio.h"
#include <array>
#include <cmath>

#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif

namespace {
    struct OutlinePreset {
        const char* name;
        uint32_t outlineMode;
        uint32_t hybridColorSource;
        float hybridColorWeight;
        float hybridDepthWeight;
        float outlineStrength;
        float outlineThreshold;
        float outlineSoftness;
        float outlineThickness;
        std::array<float, 4> outlineColor;
    };

    constexpr OutlinePreset kOutlinePresets[] = {
        { "Balanced", 4u, 2u, 1.00f, 1.00f, 2.40f, 0.050f, 0.025f, 1.10f, { 0.02f, 0.02f, 0.02f, 1.0f } },
        { "Color Emphasis", 4u, 2u, 1.35f, 0.45f, 2.80f, 0.055f, 0.025f, 1.15f, { 0.03f, 0.03f, 0.03f, 1.0f } },
        { "Depth Emphasis", 4u, 1u, 0.55f, 1.45f, 2.60f, 0.060f, 0.030f, 1.20f, { 0.01f, 0.01f, 0.01f, 1.0f } },
        { "Soft Outline", 4u, 1u, 0.85f, 0.75f, 1.70f, 0.035f, 0.100f, 1.60f, { 0.08f, 0.08f, 0.08f, 1.0f } },
    };
}

void GameScene::Initialize() {
    auto modelManager = ModelManager::GetInstance();
    auto texManager = TextureManager::GetInstance();
    auto particleManager = ParticleManager::GetInstance();
    auto object3dCommon = MyGame::GetInstance()->GetObject3dCommon();
    auto spriteCommon = MyGame::GetInstance()->GetSpriteCommon();

    // --- モデル取得・生成 ---
    modelFence_ = modelManager->FindModel("resources/obj/fence/fence.obj");

    modelSphere_ = modelManager->CreateSphere("InternalSphere", 16);

    camera_ = std::make_unique<Camera>();
    camera_->SetTranslate({ 0.0f, 2.0f, -10.0f });
    camera_->SetRotate({ 0.1f, 0.0f, 0.0f });

    skybox_ = std::make_unique<Skybox>();
    skybox_->Initialize(MyGame::GetInstance()->GetSkyboxCommon());
    skybox_->SetCamera(camera_.get());
    skybox_->SetScale(skyboxScale_);
    skybox_->SetTexture(skyboxTexturePath_);
    skyboxTextureIndex_ = texManager->GetTextureIndexByFilePath(skyboxTexturePath_);
    skyboxTranslate_ = camera_->GetTranslate();
    skybox_->SetTranslate(skyboxTranslate_);

    object3d_ = std::make_unique<Object3d>();
    object3d_->Initialize(object3dCommon);
    object3d_->SetModel(modelFence_);
    object3d_->SetTranslate({ -2.0f, 0.0f, 0.0f });
    object3d_->SetCamera(camera_.get());
    object3d_->SetEnvironmentTextureIndex(skyboxTextureIndex_);
    object3d_->SetEnvironmentMapEnabled(false);

    object3dSphere_ = std::make_unique<Object3d>();
    object3dSphere_->Initialize(object3dCommon);
    object3dSphere_->SetModel(modelSphere_); // 生成した球体をセット
    object3dSphere_->SetTranslate({ 2.0f, 0.0f, 0.0f });
    object3dSphere_->SetCamera(camera_.get());
    object3dSphere_->SetEnvironmentTextureIndex(skyboxTextureIndex_);
    object3dSphere_->SetEnvironmentMapEnabled(isSphereEnvironmentMapEnabled_);
    object3dSphere_->SetEnvironmentMapIntensity(sphereEnvironmentMapIntensity_);

    texManager->LoadTexture("resources/obj/axis/uvChecker.png");
    texManager->LoadTexture("resources/obj/fence/fence.png");
    texManager->LoadTexture("resources/obj/monsterBall/monsterBall.png");
    texManager->LoadTexture("resources/particle/circle2.png");

    texIndexUvChecker_ = texManager->GetTextureIndexByFilePath("resources/obj/axis/uvChecker.png");
    texIndexFence_ = texManager->GetTextureIndexByFilePath("resources/obj/fence/fence.png");
    texIndexMonsterBall_ = texManager->GetTextureIndexByFilePath("resources/obj/monsterBall/monsterBall.png");

    if (modelSphere_) {
        // 初期テクスチャをモンスターボールに設定
        modelSphere_->SetTextureIndex(texIndexMonsterBall_);
    }

    primitivePreviewObjects_.clear();
    primitivePreviewObjects_.reserve(8);

    auto createPrimitivePreview = [&](Model* model, const Vector3& translate, const Vector3& rotate, const Vector3& scale) {
        if (!model) {
            return;
        }

        auto preview = std::make_unique<Object3d>();
        preview->Initialize(object3dCommon);
        preview->SetModel(model);
        preview->SetCamera(camera_.get());
        preview->SetTranslate(translate);
            preview->SetRotate(rotate);
        preview->SetScale(scale);
        preview->SetEnvironmentMapEnabled(false);
        primitivePreviewObjects_.push_back(std::move(preview));
        };

    createPrimitivePreview(modelManager->CreatePlane("PrimitivePlane"), { -4.5f, -1.0f, 3.0f }, { 0.0f, 0.0f, 0.0f }, { 1.4f, 1.4f, 1.4f });
    createPrimitivePreview(modelManager->CreateCircle("PrimitiveCircle", 32), { -1.5f, -1.0f, 3.0f }, { 0.0f, 0.0f, 0.0f }, { 1.2f, 1.2f, 1.2f });
    createPrimitivePreview(modelManager->CreateRing("PrimitiveRing", 32, 0.45f, 1.0f), { 1.5f, -1.0f, 3.0f }, { 0.0f, 0.0f, 0.0f }, { 1.2f, 1.2f, 1.2f });
    createPrimitivePreview(modelManager->CreateTriangle("PrimitiveTriangle"), { 4.5f, -1.0f, 3.0f }, { 0.0f, 0.0f, 0.0f }, { 1.4f, 1.4f, 1.4f });
    createPrimitivePreview(modelManager->CreateBox("PrimitiveBox"), { -4.5f, 0.9f, 6.0f }, { 0.35f, 0.45f, 0.0f }, { 0.9f, 0.9f, 0.9f });
    createPrimitivePreview(modelManager->CreateCylinder("PrimitiveCylinder", 32), { -1.5f, 0.9f, 6.0f }, { 0.1f, 0.35f, 0.0f }, { 0.85f, 0.85f, 0.85f });
    createPrimitivePreview(modelManager->CreateCone("PrimitiveCone", 32), { 1.5f, 0.9f, 6.0f }, { 0.1f, 0.35f, 0.0f }, { 0.85f, 0.85f, 0.85f });
    createPrimitivePreview(modelManager->CreateTorus("PrimitiveTorus", 32, 16), { 4.5f, 0.9f, 6.0f }, { 0.6f, 0.3f, 0.0f }, { 1.0f, 1.0f, 1.0f });

    debugSprite_ = std::make_unique<Sprite>();
    debugSprite_->Initialize(spriteCommon);
    debugSprite_->SetTexture("resources/obj/axis/uvChecker.png");
    debugSprite_->SetPosition({ 100.0f, 100.0f });
    debugSprite_->SetSize({ 100.0f, 100.0f });

    particleManager->SetTexture(particleTexturePath_);
}

void GameScene::Finalize() {}

void GameScene::Update() {
    auto input = MyGame::GetInstance()->GetInput();
    auto particleManager = ParticleManager::GetInstance();
    auto& postEffectParams = MyGame::GetInstance()->GetDxCommon()->GetPostEffectParameters();
    auto& hitEffectParams = particleManager->GetHitEffectParams();
    auto& fireballEffectParams = particleManager->GetFireballEffectParams();
    auto& windEffectParams = particleManager->GetWindEffectParams();
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
    if (isSkyboxFollowCamera_) {
        skyboxTranslate_ = camera_->GetTranslate();
    }
    skybox_->SetCamera(camera_.get());
    skybox_->SetScale(skyboxScale_);
    skybox_->SetTranslate(skyboxTranslate_);
    skybox_->Update();
    object3dSphere_->SetEnvironmentMapEnabled(isSphereEnvironmentMapEnabled_);
    object3dSphere_->SetEnvironmentMapIntensity(sphereEnvironmentMapIntensity_);
    object3d_->Update();
    object3dSphere_->Update();
    for (auto& primitivePreviewObject : primitivePreviewObjects_) {
        primitivePreviewObject->Update();
    }
    debugSprite_->Update();

    if (input->PushKey(DIK_SPACE)) {
        particleManager->Emit("Hit", object3dSphere_->GetTransform().translate, hitEffectParams.spawnCount);
    }

    if (input->PushKey(DIK_H)) {
        particleManager->Emit("Hit", object3dSphere_->GetTransform().translate, hitEffectParams.spawnCount);
    }

    if (input->TriggerKey(DIK_P)) {
        particleManager->Emit(particleTexturePath_, object3dSphere_->GetTransform().translate, 1);
    }

    particleManager->Update(camera_.get());

#ifdef _DEBUG
    ImGui::SetNextWindowSize(ImVec2(500, 200), ImGuiCond_Once);
    ImGui::Begin("DebugText");
    Vector2 spritePos = debugSprite_->GetPosition();
    ImGui::DragFloat2("Sprite Pos", &spritePos.x, 1.0f, -9999.0f, 9999.0f, "%4.1f");
    debugSprite_->SetPosition(spritePos);
    ImGui::End();

    auto DrawEffectParamsUI = [](const char* label, ParticleManager::EffectParams& params) {
        std::string prefix = label;

        int spawnCount = static_cast<int>(params.spawnCount);
        if (ImGui::DragInt((prefix + " Spawn Count").c_str(), &spawnCount, 1.0f, 1, 100)) {
            if (spawnCount < 1) {
                spawnCount = 1;
            }
            params.spawnCount = static_cast<uint32_t>(spawnCount);
        }

        ImGui::DragFloat2((prefix + " Scale X").c_str(), &params.scaleXRange.x, 0.01f, 0.01f, 4.0f, "%.2f");
        ImGui::DragFloat2((prefix + " Scale Y").c_str(), &params.scaleYRange.x, 0.01f, 0.01f, 6.0f, "%.2f");
        ImGui::DragFloat2((prefix + " Lifetime").c_str(), &params.lifeTimeRange.x, 0.01f, 0.01f, 3.0f, "%.2f");
        ImGui::DragFloat2((prefix + " Speed").c_str(), &params.speedRange.x, 0.01f, 0.0f, 1.0f, "%.2f");
        ImGui::DragFloat2((prefix + " Rotate Z").c_str(), &params.rotateZRange.x, 0.01f, -6.29f, 6.29f, "%.2f");

        Vector3 colorMin = { params.colorRRange.x, params.colorGRange.x, params.colorBRange.x };
        Vector3 colorMax = { params.colorRRange.y, params.colorGRange.y, params.colorBRange.y };
        if (ImGui::ColorEdit3((prefix + " Color Min").c_str(), &colorMin.x)) {
            params.colorRRange.x = colorMin.x;
            params.colorGRange.x = colorMin.y;
            params.colorBRange.x = colorMin.z;
        }
        if (ImGui::ColorEdit3((prefix + " Color Max").c_str(), &colorMax.x)) {
            params.colorRRange.y = colorMax.x;
            params.colorGRange.y = colorMax.y;
            params.colorBRange.y = colorMax.z;
        }
        };

    ImGui::Begin("Game Scene Menu");
    ImGui::Text("Press [T] to return to Title");
    ImGui::SeparatorText("Camera");
    Vector3 camTrans = camera_->GetTranslate();
    if (ImGui::DragFloat3("Cam Pos", &camTrans.x, 0.1f)) camera_->SetTranslate(camTrans);

    ImGui::SeparatorText("Skybox");
    ImGui::Checkbox("Show Skybox", &isSkyboxVisible_);
    ImGui::Checkbox("Follow Camera", &isSkyboxFollowCamera_);
    ImGui::DragFloat3("Skybox Scale", &skyboxScale_.x, 1.0f, 1.0f, 1000.0f, "%.1f");
    ImGui::TextWrapped("DDS: %s", skyboxTexturePath_.c_str());
    ImGui::Text("TextureIndex: %u", skyboxTextureIndex_);

    ImGui::SeparatorText("Environment Map");
    ImGui::Checkbox("Reflect Sphere", &isSphereEnvironmentMapEnabled_);
    ImGui::SliderFloat("Reflect Strength", &sphereEnvironmentMapIntensity_, 0.0f, 1.0f, "%.2f");
    ImGui::TextWrapped("Cubemap DDS: %s", skyboxTexturePath_.c_str());
    ImGui::Text("Cubemap TextureIndex: %u", skyboxTextureIndex_);

    ImGui::SeparatorText("Post Effects");
    auto DrawPostEffectUI = [](const char* label, uint32_t& enabled, float& intensity) {
        bool isEnabled = enabled != 0;
        if (ImGui::Checkbox(label, &isEnabled)) {
            enabled = isEnabled ? 1u : 0u;
        }
        std::string sliderLabel = std::string(label) + " Strength";
        ImGui::SliderFloat(sliderLabel.c_str(), &intensity, 0.0f, 1.0f, "%.2f");
        };
    auto ApplyOutlinePreset = [&](const OutlinePreset& preset) {
        postEffectParams.outlineMode = preset.outlineMode;
        postEffectParams.hybridColorSource = preset.hybridColorSource;
        postEffectParams.hybridColorWeight = preset.hybridColorWeight;
        postEffectParams.hybridDepthWeight = preset.hybridDepthWeight;
        postEffectParams.outlineIntensity = preset.outlineStrength;
        postEffectParams.outlineThreshold = preset.outlineThreshold;
        postEffectParams.outlineSoftness = preset.outlineSoftness;
        postEffectParams.outlineThickness = preset.outlineThickness;
        postEffectParams.outlineColor = preset.outlineColor;
    };
    bool gaussianEnabled = postEffectParams.gaussianEnabled != 0;
    if (ImGui::Checkbox("Gaussian", &gaussianEnabled)) {
        postEffectParams.gaussianEnabled = gaussianEnabled ? 1u : 0u;
    }
    ImGui::SliderFloat("Gaussian Strength", &postEffectParams.gaussianIntensity, 0.0f, 4.0f, "%.2f");
    bool outlineEnabled = postEffectParams.outlineMode != 0;
    if (ImGui::Checkbox("Outline", &outlineEnabled)) {
        if (!outlineEnabled) {
            postEffectParams.outlineMode = 0;
        } else if (postEffectParams.outlineMode == 0) {
            postEffectParams.outlineMode = 1;
        }
    }
    const char* outlineModeNames[] = { "Off", "ColorDiff8", "Sobel", "Depth", "Hybrid" };
    int outlineMode = static_cast<int>(postEffectParams.outlineMode);
    if (ImGui::Combo("Outline Mode", &outlineMode, outlineModeNames, IM_ARRAYSIZE(outlineModeNames))) {
        postEffectParams.outlineMode = static_cast<uint32_t>(outlineMode);
    }
    static int outlinePresetIndex = 0;
    const char* outlinePresetNames[] = { "Balanced", "Color Emphasis", "Depth Emphasis", "Soft Outline" };
    if (ImGui::Combo("Outline Preset", &outlinePresetIndex, outlinePresetNames, IM_ARRAYSIZE(outlinePresetNames))) {
        ApplyOutlinePreset(kOutlinePresets[outlinePresetIndex]);
    }
    ImGui::SliderFloat("Outline Strength", &postEffectParams.outlineIntensity, 0.0f, 10.0f, "%.2f");
    ImGui::SliderFloat("Outline Thickness", &postEffectParams.outlineThickness, 0.5f, 4.0f, "%.2f");
    ImGui::SliderFloat("Outline Threshold", &postEffectParams.outlineThreshold, 0.0f, 1.5f, "%.3f");
    ImGui::SliderFloat("Outline Softness", &postEffectParams.outlineSoftness, 0.001f, 1.0f, "%.3f");
    ImGui::SliderFloat("Outline Depth Threshold", &postEffectParams.outlineDepthThreshold, 0.0001f, 0.05f, "%.4f");
    ImGui::SliderFloat("Outline Depth Strength", &postEffectParams.outlineDepthStrength, 0.0f, 50.0f, "%.2f");
    if (postEffectParams.outlineMode == 4) {
        const char* hybridColorSourceNames[] = { "ColorDiff8", "Sobel" };
        int hybridColorSourceIndex = (postEffectParams.hybridColorSource == 1u) ? 0 : 1;
        if (ImGui::Combo("Hybrid Color Source", &hybridColorSourceIndex, hybridColorSourceNames, IM_ARRAYSIZE(hybridColorSourceNames))) {
            postEffectParams.hybridColorSource = (hybridColorSourceIndex == 0) ? 1u : 2u;
        }
        ImGui::SliderFloat("Hybrid Color Weight", &postEffectParams.hybridColorWeight, 0.0f, 2.0f, "%.2f");
        ImGui::SliderFloat("Hybrid Depth Weight", &postEffectParams.hybridDepthWeight, 0.0f, 2.0f, "%.2f");
    }
    ImGui::ColorEdit4("Outline Color", postEffectParams.outlineColor.data());
    DrawPostEffectUI("Grayscale", postEffectParams.grayscaleEnabled, postEffectParams.grayscaleIntensity);
    DrawPostEffectUI("Sepia", postEffectParams.sepiaEnabled, postEffectParams.sepiaIntensity);
    DrawPostEffectUI("Invert", postEffectParams.invertEnabled, postEffectParams.invertIntensity);
    DrawPostEffectUI("Vignette", postEffectParams.vignetteEnabled, postEffectParams.vignetteIntensity);
    DrawPostEffectUI("Smoothing", postEffectParams.smoothingEnabled, postEffectParams.smoothingIntensity);

    ImGui::SeparatorText("Primitive Preview");
    ImGui::Checkbox("Show Primitive Preview", &isPrimitivePreviewVisible_);
    ImGui::Text("Front Row : Plane / Circle / Ring / Triangle");
    ImGui::Text("Back Row  : Box / Cylinder / Cone / Torus");

    ImGui::SeparatorText("Particle Texture");
    const char* particleTextureNames[] = { "uvChecker", "Circle2", "Fence" };
    const char* particleTexturePaths[] = {
        "resources/obj/axis/uvChecker.png",
        "resources/particle/circle2.png",
        "resources/obj/fence/fence.png"
    };
    if (ImGui::Combo("Particle Texture", &currentParticleTexture_, particleTextureNames, IM_ARRAYSIZE(particleTextureNames))) {
        particleTexturePath_ = particleTexturePaths[currentParticleTexture_];
        particleManager->SetTexture(particleTexturePath_);
    }
    ImGui::TextWrapped("Particle Texture Path: %s", particleTexturePath_.c_str());

    ImGui::SeparatorText("Hit Effect");
    ImGui::TextWrapped("Main submission target: plane billboard particles stretched into hit streaks.");
    if (ImGui::Button("Emit Hit")) {
        particleManager->Emit("Hit", object3dSphere_->GetTransform().translate, hitEffectParams.spawnCount);
    }
    ImGui::Text("Hit Trigger: [Space] / [H]");

    ImGui::SeparatorText("Hit Params");
    DrawEffectParamsUI("Hit", hitEffectParams);

    if (ImGui::CollapsingHeader("Other Effects (Optional)")) {
        if (ImGui::Button("Emit Fireball")) {
            particleManager->Emit("Fireball", object3dSphere_->GetTransform().translate, fireballEffectParams.spawnCount);
        }
        ImGui::SameLine();
        if (ImGui::Button("Emit Wind")) {
            particleManager->Emit("Wind", object3dSphere_->GetTransform().translate, windEffectParams.spawnCount);
        }

        ImGui::SeparatorText("Fireball Params");
        DrawEffectParamsUI("Fireball", fireballEffectParams);

        ImGui::SeparatorText("Wind Params");
        DrawEffectParamsUI("Wind", windEffectParams);
    }

    ImGui::SeparatorText("Particle Smoke Test");
    if (ImGui::Button("Emit Basic Particle")) {
        particleManager->Emit(particleTexturePath_, object3dSphere_->GetTransform().translate, 1);
    }
    ImGui::Text("Trigger: [P]");

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
    auto skyboxCommon = MyGame::GetInstance()->GetSkyboxCommon();
    auto particleManager = ParticleManager::GetInstance();
    auto spriteCommon = MyGame::GetInstance()->GetSpriteCommon();

    if (isSkyboxVisible_) {
        skyboxCommon->CommonDrawSetting();
        skybox_->Draw();
    }

    object3dCommon->CommonDrawSetting((Object3dCommon::BlendMode)currentBlendMode_);

    object3d_->Draw();
    object3dSphere_->Draw();
    if (isPrimitivePreviewVisible_) {
        for (auto& primitivePreviewObject : primitivePreviewObjects_) {
            primitivePreviewObject->Draw();
        }
    }

    particleManager->Draw();

    spriteCommon->CommonDrawSetting();
    debugSprite_->Draw();
}
