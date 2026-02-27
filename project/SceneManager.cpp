#include "SceneManager.h"

SceneManager* SceneManager::instance_ = nullptr;

SceneManager* SceneManager::GetInstance() {
    if (instance_ == nullptr) {
        instance_ = new SceneManager();
    }
    return instance_;
}

void SceneManager::ChangeScene(IScene* newScene) {
    // 次のシーンを予約しておく（実行中のUpdate途中で破棄されないようにするため）
    nextScene_ = newScene;
}

void SceneManager::Update() {
    // シーン切り替え予約があれば切り替える
    if (nextScene_) {
        // 古いシーンの終了処理
        if (currentScene_) {
            currentScene_->Finalize();
            delete currentScene_;
        }

        // 新しいシーンを現在のシーンにする
        currentScene_ = nextScene_;
        nextScene_ = nullptr;

        // 新しいシーンの初期化
        currentScene_->Initialize();
    }

    // 現在のシーンの更新処理
    if (currentScene_) {
        currentScene_->Update();
    }
}

void SceneManager::Draw() {
    if (currentScene_) {
        currentScene_->Draw();
    }
}

void SceneManager::Finalize() {
    if (currentScene_) {
        currentScene_->Finalize();
        delete currentScene_;
        currentScene_ = nullptr;
    }

    delete instance_;
    instance_ = nullptr;
}