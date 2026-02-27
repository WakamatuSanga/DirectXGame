#pragma once
#include "WinApp.h"
#include "DirectXCommon.h"

// デバッグ時のみ ImGui のヘッダを含める
#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#endif

class ImGuiManager {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(WinApp* winApp, DirectXCommon* dxCommon);

	/// <summary>
	/// ImGui受付開始
	/// </summary>
	void Begin();

	/// <summary>
	/// ImGui受付終了
	/// </summary>
	void End();

	/// <summary>
	/// 画面への描画
	/// </summary>
	void Draw();

	/// <summary>
	/// 解放
	/// </summary>
	void Finalize();

private:
#ifdef _DEBUG
	// DirectXCommonのポインタを保持しておく
	DirectXCommon* dxCommon_ = nullptr;
#endif
};