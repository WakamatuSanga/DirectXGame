#pragma once
#include "Object3dCommon.h"

class Object3d
{
public:
	// 初期化
	void Initialize(Object3dCommon* object3dCommon);

	// 更新
	void Update();

	// 描画
	void Draw();

private:
	Object3dCommon* object3dCommon_ = nullptr;

	// 今後ここにトランスフォームや頂点バッファなどを持たせます
};