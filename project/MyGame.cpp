#include "MyGame.h"
#include "SceneManager.h"
#include "TitleScene.h" // 最初のシーン

MyGame* MyGame::instance_ = nullptr;

MyGame* MyGame::GetInstance() {
    if (instance_ == nullptr) {
        instance_ = new MyGame();
    }
    return instance_;
}

void MyGame::Initialize() {
    // --- 基盤初期化 ---
    winApp_ = new WinApp();
    winApp_->Initialize();

    dxCommon_ = new DirectXCommon();
    dxCommon_->Initialize(winApp_);

    srvManager_ = SrvManager::GetInstance();
    srvManager_->Initialize(dxCommon_);

    imguiManager_ = new ImGuiManager();
    imguiManager_->Initialize(winApp_, dxCommon_);

    texManager_ = TextureManager::GetInstance();
    texManager_->Initialize(dxCommon_, srvManager_);

    spriteCommon_ = new SpriteCommon();
    spriteCommon_->Initialize(dxCommon_);

    object3dCommon_ = new Object3dCommon();
    object3dCommon_->Initialize(dxCommon_);

    modelManager_ = ModelManager::GetInstance();
    modelManager_->Initialize(dxCommon_);

    input_ = new Input();
    input_->Initialize(winApp_);

    particleManager_ = ParticleManager::GetInstance();
    particleManager_->Initialize(dxCommon_, srvManager_);

    audio_ = Audio::GetInstance();
    audio_->Initialize();

    // --- テクスチャ・モデル・音声の事前ロード ---
    modelManager_->LoadModel("resources/obj/fence/fence.obj");
    texManager_->LoadTexture("resources/obj/fence/fence.png");
    texManager_->LoadTexture("resources/obj/axis/uvChecker.png");
    audio_->LoadAudio("resources/sounds/Alarm01.mp3");

    // --- 最初のシーンをセット ---
    SceneManager::GetInstance()->ChangeScene(new TitleScene());
}

void MyGame::Finalize() {
    // シーンマネージャーの終了（ここで現在のシーンも破棄される）
    SceneManager::GetInstance()->Finalize();

    // --- エンジン基盤の終了処理 ---
    texManager_->ReleaseIntermediateResources();
    particleManager_->Finalize();
    imguiManager_->Finalize();
    texManager_->Finalize();
    modelManager_->Finalize();

    delete imguiManager_;
    delete object3dCommon_;
    delete spriteCommon_;
    delete dxCommon_;
    delete winApp_;
    delete input_;

    audio_->Finalize();

    delete instance_;
    instance_ = nullptr;
}

void MyGame::Run() {
    // メインループ
    while (true) {
        if (winApp_->ProcessMessage()) break;

        Update();
        Draw();
    }
}

void MyGame::Update() {
    // ImGuiと入力の受付
    imguiManager_->Begin();
    input_->Update();

    // シーンマネージャーの更新処理（各シーンのUpdateが呼ばれる）
    SceneManager::GetInstance()->Update();

    imguiManager_->End();
}

void MyGame::Draw() {
    // 描画前処理
    dxCommon_->PreDraw();
    srvManager_->PreDraw();

    // シーンマネージャーの描画処理（各シーンのDrawが呼ばれる）
    SceneManager::GetInstance()->Draw();

    // ImGui描画と描画後処理
    imguiManager_->Draw();
    dxCommon_->PostDraw();
}