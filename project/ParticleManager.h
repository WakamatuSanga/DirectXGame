#pragma once
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "Camera.h"
#include "Matrix4x4.h"
#include <wrl.h>
#include <list>
#include <string>
#include <memory>

class ParticleManager {
    friend struct std::default_delete<ParticleManager>;

public:
    struct Particle {
        Transform transform;
        Vector3 velocity;
        Vector4 color;
        float lifeTime;
        float currentTime;
    };

    struct ParticleForGPU {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Vector4 color;
    };

public:
    static ParticleManager* GetInstance();

    void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);
    void Update(Camera* camera);
    void Draw();
    void Finalize();

    void Emit(const std::string& name, const Vector3& pos, uint32_t count);

private:
    ParticleManager() = default;
    ~ParticleManager() = default;
    ParticleManager(const ParticleManager&) = delete;
    ParticleManager& operator=(const ParticleManager&) = delete;

    void CreateRootSignature();
    void CreatePipelineState();
    void CreateModel();

    static std::unique_ptr<ParticleManager> instance_;

    DirectXCommon* dxCommon_ = nullptr;
    SrvManager* srvManager_ = nullptr;

    std::list<Particle> particles_;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

    struct VertexData {
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
    };
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_;
    ParticleForGPU* instancingData_ = nullptr;

    uint32_t srvIndex_ = 0;
    std::string textureName_;
    const uint32_t kMaxInstance = 100;
};