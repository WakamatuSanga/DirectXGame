#pragma once

#include "WinApp.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "SpriteCommon.h"
#include "Object3dCommon.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "Audio.h"

// ゲームエンジン全体を管理するクラス（シングルトン化）
class MyGame {
public:
    static MyGame* GetInstance();

    // 初期化、実行、終了
    void Initialize();
    void Finalize();
    void Run();

    // 各シーンから基盤システムへアクセスするためのゲッター
    Input* GetInput() const { return input_; }
    DirectXCommon* GetDxCommon() const { return dxCommon_; }
    Object3dCommon* GetObject3dCommon() const { return object3dCommon_; }
    SpriteCommon* GetSpriteCommon() const { return spriteCommon_; }

private:
    MyGame() = default;
    ~MyGame() = default;
    MyGame(const MyGame&) = delete;
    MyGame& operator=(const MyGame&) = delete;

    static MyGame* instance_;

    // 毎フレームの更新と描画
    void Update();
    void Draw();

private:
    // --- 基盤システムのみ保持する ---
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
};