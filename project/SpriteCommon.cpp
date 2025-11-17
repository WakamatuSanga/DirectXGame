#include "SpriteCommon.h"
#include "DirectXCommon.h"
#include <cassert>

void SpriteCommon::Initialize(DirectXCommon* dxCommon) {
    assert(dxCommon);
    dxCommon_ = dxCommon;

    // ★ここに後で「スプライト共通のパイプライン設定」などを追加していく予定
}
