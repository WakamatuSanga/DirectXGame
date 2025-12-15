#include "Object3d.h"
#include <cassert>

void Object3d::Initialize(Object3dCommon* object3dCommon)
{
	assert(object3dCommon);
	object3dCommon_ = object3dCommon;
}
void Object3d::Update()
{
	// 今後ここに行列計算などを移設します
}

void Object3d::Draw()
{
	// 今後ここにDrawCallを移設します
}