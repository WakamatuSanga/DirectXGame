#include "TitleScene.h"
#include "SceneManager.h"
#include "GameScene.h" // 次のシーン
#include "MyGame.h"    // 入力取得のため

// デバッグ時のみImGuiのヘッダをインクルード
#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif

void TitleScene::Initialize() {
    // タイトルシーンに入った時の初期化処理
}

void TitleScene::Finalize() {
    // タイトルシーンを抜ける時の終了処理
}

void TitleScene::Update() {
    // --- デバッグUI表示 ---
#ifdef _DEBUG
    ImGui::Begin("Title Scene");
    ImGui::Text("This is Title Scene.");
    ImGui::Text("Press [SPACE] to Start Game!");
    ImGui::End();
#endif

    // スペースキーを押したら、GameScene へ切り替える（状態の移行）
    if (MyGame::GetInstance()->GetInput()->PushKey(DIK_SPACE)) {
        SceneManager::GetInstance()->ChangeScene(new GameScene());
    }
}

void TitleScene::Draw() {
    // タイトル画面用のスプライトや文字があればここで描画する
}