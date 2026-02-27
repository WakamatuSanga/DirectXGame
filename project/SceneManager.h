#pragma once
#include "IScene.h"

// シーンを管理するクラス（StateパターンのContext）
class SceneManager {
public:
    static SceneManager* GetInstance();

    // 毎フレームの更新と描画
    void Update();
    void Draw();
    void Finalize();

    // 次のシーンを予約する
    void ChangeScene(IScene* newScene);

private:
    SceneManager() = default;
    ~SceneManager() = default;
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

private:
    static SceneManager* instance_;

    IScene* currentScene_ = nullptr; // 現在のシーン(State)
    IScene* nextScene_ = nullptr;    // 次のシーン(切り替え予約用)
};