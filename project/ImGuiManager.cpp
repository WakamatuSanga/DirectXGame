#include "ImGuiManager.h"

// デバッグ時のみインクルード
#ifdef _DEBUG
#include "SrvManager.h"
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#endif

void ImGuiManager::Initialize([[maybe_unused]] WinApp* winApp, [[maybe_unused]] DirectXCommon* dxCommon) {
#ifdef _DEBUG
	// メンバ変数に保持（デバッグ時のみ使用）
	dxCommon_ = dxCommon;

	// ImGuiのコンテキストを生成
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	//ImGuiIO& io = ImGui::GetIO();
	//(void)io;

	//// Win32用初期化
	ImGui_ImplWin32_Init(winApp->GetHwnd());

	// ★重要: SrvManagerからImGui用のSRVインデックスを1つ確保する
	// これを行わないと、テクスチャとImGuiのフォントが衝突してバグや文字化けの原因になります
	SrvManager* srvManager = SrvManager::GetInstance();
	uint32_t useIndex = srvManager->Allocate();

	// 確保したインデックスのハンドルを取得
	ImGui_ImplDX12_Init(
		dxCommon->GetDevice(),
		static_cast<int>(dxCommon->GetBackBufferCount()),
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		srvManager->GetSrvDescriptorHeap(),
		srvManager->GetCPUDescriptorHandle(useIndex), // 確保した場所のCPUハンドル
		srvManager->GetGPUDescriptorHandle(useIndex)  // 確保した場所のGPUハンドル
	);
#endif
}

void ImGuiManager::Begin() {
#ifdef _DEBUG
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
#endif
}

void ImGuiManager::End() {
#ifdef _DEBUG
	ImGui::Render();
#endif
}

void ImGuiManager::Draw() {
#ifdef _DEBUG
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

	// デスクリプタヒープの設定
	// (既に描画処理で設定されている場合が多いですが、念のため再設定)
	ID3D12DescriptorHeap* descriptorHeaps[] = { SrvManager::GetInstance()->GetSrvDescriptorHeap() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
#endif
}

void ImGuiManager::Finalize() {
#ifdef _DEBUG
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif
}