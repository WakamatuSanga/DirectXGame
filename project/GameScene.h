#pragma once
#include "IScene.h"
#include "Model.h"
#include "Camera.h"
#include "Object3d.h"
#include "Sprite.h"
#include <memory>

class GameScene : public IScene {
public:
    void Initialize() override;
    void Update() override;
    void Draw() override;
    void Finalize() override;

private:
    // --- 自分が所有権を持つデータは unique_ptr ---
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Object3d> object3d_;
    std::unique_ptr<Sprite> debugSprite_;

    // --- 他人が所有しているデータは 生ポインタ (オブザーバー) ---
    Model* modelFence_ = nullptr; // ModelManagerが所有している

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