#include "Particle.hlsli"

// C++のRootParameter[0] (StructuredBuffer) を受け取る
// register(t0) は頂点シェーダーでのみ見えるように設定されている
StructuredBuffer<ParticleForGPU> gParticle : register(t0);

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input, uint32_t instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
	
	// インスタンスIDを使って現在のパーティクル情報を取得
    ParticleForGPU particle = gParticle[instanceId];

	// 座標変換
    output.position = mul(input.position, particle.WVP);
    output.texcoord = input.texcoord;
	
	// パーティクルの色を出力に渡す
    output.color = particle.color;
	
    return output;
}