#pragma once
#include "IScene.h"
#include "Model.h"
#include "Camera.h"
#include "Object3d.h"
#include "Sprite.h"

// ゲーム本編のシーン
class GameScene : public IScene {
public:
    void Initialize() override;
    void Update() override;
    void Draw() override;
    void Finalize() override;

private:
    // --- ゲーム固有データ ---
    Model* modelFence_ = nullptr;
    Camera* camera_ = nullptr;
    Object3d* object3d_ = nullptr;

    // ImGuiでの操作対象となるスプライト
    Sprite* debugSprite_ = nullptr;

    uint32_t texIndexUvChecker_ = 0;
    uint32_t texIndexFence_ = 0;

    int currentModelTexture_ = 1;
    int currentBlendMode_ = 0;

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