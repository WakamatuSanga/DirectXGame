#include "SceneManager.h"

std::unique_ptr<SceneManager> SceneManager::instance_ = nullptr;

SceneManager* SceneManager::GetInstance() {
    if (!instance_) {
        instance_.reset(new SceneManager());
    }
    return instance_.get();
}

void SceneManager::ChangeScene(std::unique_ptr<IScene> newScene) {
    // 所有権の移動
    nextScene_ = std::move(newScene);
}

void SceneManager::Update() {
    // シーン切り替え予約があれば切り替える
    if (nextScene_) {
        // 古いシーンの終了処理
        if (currentScene_) {
            currentScene_->Finalize();
            // unique_ptrの書き換えにより、古いシーンは自動でdeleteされます
        }

        // 新しいシーンを現在のシーンにする
        currentScene_ = std::move(nextScene_);

        // 新しいシーンの初期化
        currentScene_->Initialize();
    }

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
        currentScene_.reset(); // 自動delete
    }
}