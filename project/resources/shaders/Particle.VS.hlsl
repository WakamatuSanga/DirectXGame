// Particle.VS.hlsl (この内容をそのままコピー＆ペーストしてください)

#include "Particle.hlsli"

struct TransformationMatrix
{
    float32_t4x4 WVP;
    float32_t4x4 World;
};

StructuredBuffer<TransformationMatrix> gTransformationMatrices : register(t0);

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input, uint32_t instanceID : SV_InstanceID)
{
    VertexShaderOutput output;

    TransformationMatrix matrix=gTransformationMatrices[instanceID];

    // ★★★修正済みの正しいコード★★★
    output.position = mul(input.position,matrix.WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float32_t3x3)matrix.World));
    
    return output;
}