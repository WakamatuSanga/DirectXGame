#include "ParticleManager.h"
#include "TextureManager.h"
#include <random>
#include <cassert>
#include <vector>
#include <string>
#include <Windows.h> // MessageBox用

using namespace MatrixMath;
using namespace Microsoft::WRL;

ParticleManager* ParticleManager::instance_ = nullptr;

ParticleManager* ParticleManager::GetInstance() {
	if (instance_ == nullptr) {
		instance_ = new ParticleManager();
	}
	return instance_;
}

void ParticleManager::Initialize(DirectXCommon* dxCommon, SrvManager* srvManager) {
	assert(dxCommon);
	dxCommon_ = dxCommon;
	srvManager_ = srvManager;

	CreateRootSignature();
	CreatePipelineState();
	CreateModel();

	// インスタンシング用リソース生成
	instancingResource_ = dxCommon_->CreateBufferResource(sizeof(ParticleForGPU) * kMaxInstance);
	instancingResource_->Map(0, nullptr, reinterpret_cast<void**>(&instancingData_));

	// SRV確保
	srvIndex_ = srvManager_->Allocate();
	srvManager_->CreateSRVforStructuredBuffer(srvIndex_, instancingResource_.Get(), kMaxInstance, sizeof(ParticleForGPU));

	// デフォルトテクスチャロード
	textureName_ = "resources/obj/axis/uvChecker.png";
	TextureManager::GetInstance()->LoadTexture(textureName_);
}

void ParticleManager::Update(Camera* camera) {
	const float kDeltaTime = 1.0f / 60.0f;

	// 寿命が尽きたものを削除
	particles_.remove_if([](Particle& p) {
		return p.currentTime >= p.lifeTime;
		});

	// 移動処理
	for (auto& particle : particles_) {
		particle.currentTime += kDeltaTime;
		particle.transform.translate.x += particle.velocity.x * kDeltaTime;
		particle.transform.translate.y += particle.velocity.y * kDeltaTime;
		particle.transform.translate.z += particle.velocity.z * kDeltaTime;

		float alpha = 1.0f - (particle.currentTime / particle.lifeTime);
		particle.color.w = alpha;
	}

	// GPUデータ転送
	Matrix4x4 viewProj = camera->GetViewProjectionMatrix();
	Matrix4x4 billboardMatrix = camera->GetWorldMatrix();

	billboardMatrix.m[3][0] = 0.0f;
	billboardMatrix.m[3][1] = 0.0f;
	billboardMatrix.m[3][2] = 0.0f;

	uint32_t index = 0;
	for (auto& particle : particles_) {
		if (index >= kMaxInstance) break;

		Matrix4x4 world = MakeIdentity4x4();
		Matrix4x4 scaleMat = MakeScale(particle.transform.scale);
		Matrix4x4 translateMat = MakeTranslate(particle.transform.translate);

		world = Multipty(scaleMat, Multipty(billboardMatrix, translateMat));

		instancingData_[index].World = world;
		instancingData_[index].WVP = Multipty(world, viewProj);
		instancingData_[index].color = particle.color;

		index++;
	}
}

void ParticleManager::Draw() {
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
	UINT instanceCount = static_cast<UINT>(particles_.size());
	if (instanceCount == 0) return;
	if (instanceCount > kMaxInstance) instanceCount = kMaxInstance;

	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(pipelineState_.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

	commandList->SetGraphicsRootDescriptorTable(0, srvManager_->GetGPUDescriptorHandle(srvIndex_));

	uint32_t texIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureName_);
	commandList->SetGraphicsRootDescriptorTable(1, TextureManager::GetInstance()->GetSrvHandleGPU(texIndex));

	commandList->DrawInstanced(6, instanceCount, 0, 0);
}

void ParticleManager::Emit(const std::string& name, const Vector3& pos, uint32_t count) {
	textureName_ = name;
	TextureManager::GetInstance()->LoadTexture(name);

	std::random_device seed_gen;
	std::mt19937 engine(seed_gen());
	std::uniform_real_distribution<float> distPos(-0.5f, 0.5f);
	std::uniform_real_distribution<float> distVel(-1.0f, 1.0f);
	std::uniform_real_distribution<float> distTime(1.0f, 3.0f);

	for (uint32_t i = 0; i < count; ++i) {
		Particle p{};
		p.transform.scale = { 1.0f, 1.0f, 1.0f };
		p.transform.rotate = { 0.0f, 0.0f, 0.0f };
		p.transform.translate = {
			pos.x + distPos(engine),
			pos.y + distPos(engine),
			pos.z + distPos(engine)
		};
		p.velocity = { distVel(engine), distVel(engine), distVel(engine) };
		p.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		p.lifeTime = distTime(engine);
		p.currentTime = 0.0f;

		particles_.push_back(p);
	}
}

void ParticleManager::Finalize() {
	delete instance_;
	instance_ = nullptr;
}

void ParticleManager::CreateModel() {
	VertexData* vert = nullptr;
	vertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * 6);
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vert));

	vert[0] = { { -0.5f,  0.5f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } };
	vert[1] = { { -0.5f, -0.5f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } };
	vert[2] = { {  0.5f,  0.5f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } };
	vert[3] = { {  0.5f,  0.5f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } };
	vert[4] = { { -0.5f, -0.5f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } };
	vert[5] = { {  0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } };

	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * 6;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);
}

void ParticleManager::CreateRootSignature() {
	D3D12_ROOT_PARAMETER rootParams[2]{};

	D3D12_DESCRIPTOR_RANGE rangeInst{};
	rangeInst.BaseShaderRegister = 0;
	rangeInst.NumDescriptors = 1;
	rangeInst.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	rangeInst.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParams[0].DescriptorTable.pDescriptorRanges = &rangeInst;
	rootParams[0].DescriptorTable.NumDescriptorRanges = 1;

	D3D12_DESCRIPTOR_RANGE rangeTex{};
	rangeTex.BaseShaderRegister = 0;
	rangeTex.NumDescriptors = 1;
	rangeTex.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	rangeTex.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParams[1].DescriptorTable.pDescriptorRanges = &rangeTex;
	rootParams[1].DescriptorTable.NumDescriptorRanges = 1;

	D3D12_STATIC_SAMPLER_DESC sampler{};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootDesc{};
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootDesc.NumParameters = 2;
	rootDesc.pParameters = rootParams;
	rootDesc.NumStaticSamplers = 1;
	rootDesc.pStaticSamplers = &sampler;

	ComPtr<ID3DBlob> blob, error;
	HRESULT hr = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
	if (FAILED(hr)) {
		MessageBoxA(nullptr, (char*)error->GetBufferPointer(), "RootSignature Error", MB_OK);
		assert(false);
	}
	dxCommon_->GetDevice()->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
}

void ParticleManager::CreatePipelineState() {
	// シェーダー読み込みチェック
	auto vs = dxCommon_->CompileShader(L"resources/shaders/Particle.VS.hlsl", L"vs_6_0");
	auto ps = dxCommon_->CompileShader(L"resources/shaders/Particle.PS.hlsl", L"ps_6_0");

	if (!vs || !ps) {
		MessageBoxA(nullptr, "Particle Shader Compile Failed.\nCheck resources/shaders/Particle.VS.hlsl or PS.hlsl", "Error", MB_OK);
		assert(false);
	}

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	desc.pRootSignature = rootSignature_.Get();
	desc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
	desc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
	desc.InputLayout = { inputLayout, _countof(inputLayout) };
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.SampleDesc.Count = 1;
	desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	desc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	desc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	desc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	desc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	desc.DepthStencilState.DepthEnable = TRUE;
	desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState_));
	if (FAILED(hr)) {
		MessageBoxA(nullptr, "CreateGraphicsPipelineState Failed.\nRootSignatureとShaderの整合性を確認してください。", "Error", MB_OK);
		assert(false);
	}
}