#pragma once

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
#include "ImGuiManager.h"
#include "Audio.h"

// ゲーム全体を管理するクラス
class MyGame {
public:
    MyGame();
    ~MyGame();

    // 初期化、実行、終了
    void Initialize();
    void Finalize();
    void Run();

private:
    // 毎フレームの更新と描画
    void Update();
    void Draw();

private:
    // --- 基盤システム ---
    WinApp* winApp_ = nullptr;
    DirectXCommon* dxCommon_ = nullptr;
    SrvManager* srvManager_ = nullptr;
    ImGuiManager* imguiManager_ = nullptr;
    TextureManager* texManager_ = nullptr;
    SpriteCommon* spriteCommon_ = nullptr;
    Object3dCommon* object3dCommon_ = nullptr;
    ModelManager* modelManager_ = nullptr;
    Input* input_ = nullptr;
    ParticleManager* particleManager_ = nullptr;
    Audio* audio_ = nullptr;

    // --- ゲーム固有データ ---
    Model* modelFence_ = nullptr;
    Camera* camera_ = nullptr;
    Object3d* object3d_ = nullptr;

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