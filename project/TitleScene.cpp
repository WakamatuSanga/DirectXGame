#include "TitleScene.h"
#include "SceneManager.h"
#include "GameScene.h" 
#include "MyGame.h"    

#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif

void TitleScene::Initialize() {}

void TitleScene::Finalize() {}

void TitleScene::Update() {
#ifdef _DEBUG
    ImGui::Begin("Title Scene");
    ImGui::Text("This is Title Scene.");
    ImGui::Text("Press [SPACE] to Start Game!");
    ImGui::End();
#endif

    // unique_ptrを使ってシーン切り替え
    if (MyGame::GetInstance()->GetInput()->PushKey(DIK_SPACE)) {
        SceneManager::GetInstance()->ChangeScene(std::make_unique<GameScene>());
    }
}

void TitleScene::Draw() {}