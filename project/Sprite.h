#pragma once

class SpriteCommon;

// スプライト1枚分
class Sprite {
public:
    // 初期化（SpriteCommon を受け取る）
    void Initialize(SpriteCommon* spriteCommon);

    // 必要になったら使う
    void Update();
    void Draw();

private:
    // 共通部へのポインタ
    SpriteCommon* spriteCommon_ = nullptr;

    // 1枚のスプライト用データ（とりあえず最低限）
    float posX_ = 0.0f;
    float posY_ = 0.0f;
    float scaleX_ = 1.0f;
    float scaleY_ = 1.0f;
    float rotation_ = 0.0f;
};
