#pragma once
#include "IScene.h"
#include "Model.h"
#include "Camera.h"
#include "Object3d.h"
#include "Skybox.h"
#include "Sprite.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

class GameScene : public IScene {
public:
    void Initialize() override;
    void Update() override;
    void Draw() override;
    void Finalize() override;

private:
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Skybox> skybox_;
    std::unique_ptr<Object3d> object3d_;       // フェンス用
    std::unique_ptr<Object3d> object3dSphere_; // 球用
    std::unique_ptr<Sprite> debugSprite_;
    std::vector<std::unique_ptr<Object3d>> primitivePreviewObjects_;

    Model* modelFence_ = nullptr;
    Model* modelSphere_ = nullptr;

    uint32_t texIndexUvChecker_ = 0;
    uint32_t texIndexFence_ = 0;
    uint32_t texIndexMonsterBall_ = 0;
    uint32_t skyboxTextureIndex_ = 0;

    int currentModelTexture_ = 1;
    int currentParticleTexture_ = 0;
    int currentDissolveNoiseTexture_ = 0;
    int currentObjectDissolveMaskTexture_ = 0;
    int currentBlendMode_ = 0;
    int targetObjectIndex_ = 1; // 0=Fence, 1=Sphere

    bool isSkyboxVisible_ = true;
    bool isSkyboxFollowCamera_ = true;
    std::string skyboxTexturePath_ = "resources/skybox/skybox.dds";
    Vector3 skyboxScale_ = { 100.0f, 100.0f, 100.0f };
    Vector3 skyboxTranslate_ = { 0.0f, 0.0f, 0.0f };
    std::string particleTexturePath_ = "resources/obj/axis/uvChecker.png";
    std::string objectDissolveMaskTexturePath_ = "resources/postEffect/noise0.png";
    bool isSphereEnvironmentMapEnabled_ = true;
    float sphereEnvironmentMapIntensity_ = 1.0f;
    bool isObjectDissolveEnabled_ = false;
    float objectDissolveThreshold_ = 0.0f;
    float objectDissolveEdgeWidth_ = 0.05f;
    float objectDissolveEdgeGlowStrength_ = 0.5f;
    float objectDissolveEdgeNoiseStrength_ = 0.25f;
    std::array<float, 4> objectDissolveEdgeColor_ = { 1.0f, 0.5f, 0.1f, 1.0f };
    bool isObjectRandomEnabled_ = false;
    bool isObjectRandomPreview_ = true;
    float objectRandomIntensity_ = 1.0f;
    float objectRandomTime_ = 0.0f;
    bool isPrimitivePreviewVisible_ = true;

    float layoutStartX_ = -1.4f;
    float layoutStartY_ = -0.8f;
    float layoutStartZ_ = 0.0f;
    float layoutStepX_ = 0.22f;
    float layoutStepY_ = 0.11f;
    float layoutStepZ_ = 0.05f;

#ifdef _DEBUG
    const char* blendModeNames_[6] = { "Normal", "Add", "Subtract", "Multiply", "Screen", "None" };
#endif
};
