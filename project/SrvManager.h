#pragma once
#include "DirectXCommon.h"

// SRVインデックス管理クラス
class SrvManager {
public:
	// シングルトン
	static SrvManager* GetInstance();

	void Initialize(DirectXCommon* dxCommon);

	// SRV用デスクリプタヒープの確保済みインデックスを取得し、カウントを進める
	uint32_t Allocate();

	// 最大数チェック
	bool CanAllocate() const;

	// 指定したインデックスにStructuredBufferのSRVを生成する
	void CreateSRVforStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride);

	// 指定したインデックスにTexture2DのSRVを生成する
	void CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT format, UINT mipLevels);

	// 描画開始時にHeapをセットする
	void PreDraw();

	// ゲッター
	ID3D12DescriptorHeap* GetSrvDescriptorHeap() const { return dxCommon_->GetSrvDescriptorHeap(); }

	// デスクリプタハンドル取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);


private:
	SrvManager() = default;
	~SrvManager() = default;
	SrvManager(const SrvManager&) = delete;
	SrvManager& operator=(const SrvManager&) = delete;

	static SrvManager* instance_;

	DirectXCommon* dxCommon_ = nullptr;

	// 次に割り当てるSRVインデックス
	uint32_t useIndex_ = 0;

	// SRV最大数
	static const uint32_t kMaxSrvCount;
};