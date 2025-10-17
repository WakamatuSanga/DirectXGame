#include "Particle.hlsli"

struct TransformationMatrix
{
    float32_t4x4 WVP;
    float32_t4x4 World;
};

// VS でインスタンス行列配列（t0）を読む
StructuredBuffer<TransformationMatrix> gTransformationMatrix : register(t0);

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input, uint32_t instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    // 行優先（-Zpr）前提：mul(float4, float4x4)
    output.position = mul(input.position, gTransformationMatrix[instanceId].WVP);
    output.texcoord = input.texcoord;

    // 法線は World(3x3) で回して正規化
    float32_t3x3 world3x3 = (float32_t3x3) gTransformationMatrix[instanceId].World;
    output.normal = normalize(mul(input.normal, world3x3));
    return output;
}
