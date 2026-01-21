#pragma once
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "Camera.h"
#include "Matrix4x4.h"
#include <wrl.h>
#include <list>
#include <string>

// パーティクルマネージャー
class ParticleManager {
public:
	// パーティクル1粒のデータ
	struct Particle {
		Transform transform;
		Vector3 velocity;
		Vector4 color;
		float lifeTime;
		float currentTime;
	};

	// GPUに送るデータ構造
	struct ParticleForGPU {
		Matrix4x4 WVP;
		Matrix4x4 World;
		Vector4 color;
	};

public:
	// シングルトン
	static ParticleManager* GetInstance();

	void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);
	void Update(Camera* camera);
	void Draw();
	void Finalize();

	// パーティクル発生
	// name: テクスチャ名(パス), pos: 中心座標, count: 発生数
	void Emit(const std::string& name, const Vector3& pos, uint32_t count);

private:
	ParticleManager() = default;
	~ParticleManager() = default;
	ParticleManager(const ParticleManager&) = delete;
	ParticleManager& operator=(const ParticleManager&) = delete;

	void CreateRootSignature();
	void CreatePipelineState();
	void CreateModel(); // 板ポリ生成

	static ParticleManager* instance_;

	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	std::list<Particle> particles_; // パーティクルリスト

	// リソース関連
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

	// 頂点データ(共通の板ポリ)
	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	// インスタンシングリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_;
	ParticleForGPU* instancingData_ = nullptr;
	uint32_t srvIndex_ = 0; // StructuredBuffer用のSRVインデックス

	// パーティクル用テクスチャ
	// 簡易的に1種類のテクスチャを使う想定 (拡張可能)
	std::string textureName_;
	uint32_t textureSrvIndex_ = 0;

	const uint32_t kMaxInstance = 1000; // 最大数
};