#include "Sprite.h"
#include "SpriteCommon.h"
#include <cassert>

void Sprite::Initialize(SpriteCommon* spriteCommon) {
    assert(spriteCommon);
    spriteCommon_ = spriteCommon;

    // 位置やサイズの初期値（とりあえず原点・等倍）
    posX_ = 0.0f;
    posY_ = 0.0f;
    scaleX_ = 1.0f;
    scaleY_ = 1.0f;
    rotation_ = 0.0f;
}

void Sprite::Update() {
    // ★必要になったら更新処理を書く
}

void Sprite::Draw() {
    // ★次のスライド以降で、SpriteCommon と DirectXCommon を使った描画処理を追加する
}
