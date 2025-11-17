#include "Sprite.h"
#include "SpriteCommon.h"
#include "Matrix4x4.h"
#include <cassert>

using namespace MatrixMath;

void Sprite::Initialize(SpriteCommon* spriteCommon) {
    assert(spriteCommon);
    spriteCommon_ = spriteCommon;

    // 位置やサイズ、回転の初期値
    position_ = { 0.0f, 0.0f };
    size_ = { 640.0f, 360.0f };
    rotation_ = 0.0f;

    // ★頂点バッファや CBV の生成処理がすでにあるなら、
    //   そのコードは残したままで OK（この下に続けてください）。
}

void Sprite::Update() {
    // --- スライド「座標・回転・サイズの反映処理」---

    // 座標
    transform_.translate = { position_.x, position_.y, 0.0f };
    // 回転（Z軸だけ使う）
    transform_.rotate = { 0.0f, 0.0f, rotation_ };
    // サイズ = スケール
    transform_.scale = { size_.x, size_.y, 1.0f };

    // 行列を作って定数バッファに書き込む
    Matrix4x4 world = MakeAffine(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 vp = MakeIdentity4x4();          // 今はとりあえず単位行列
    Matrix4x4 wvp = Multipty(world, vp);

    if (transformationMatrixData_) {
        transformationMatrixData_->World = world;
        transformationMatrixData_->WVP = wvp;
    }

    // サイズ反映用：1x1 の板ポリにしておく（頂点が有効なときだけ）
    if (vertexData_) {
        // 左下
        vertexData_[0].position = { 0.0f, 1.0f, 0.0f, 1.0f };
        // 左上
        vertexData_[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };
        // 右下
        vertexData_[2].position = { 1.0f, 1.0f, 0.0f, 1.0f };
        // 右上
        vertexData_[3].position = { 1.0f, 0.0f, 0.0f, 1.0f };
    }
}

void Sprite::Draw() {
    // ★ここはまだ「SpriteCommon と組み合わせた描画処理」を入れる段階なので、
    //   いまは手を付けなくて大丈夫です（スライドの次の回で実装）。
}
