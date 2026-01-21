#include "Object3d.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    // 座標変換 (WVP)
    output.position = mul(input.position, gTransformationMatrix.WVP);
    
    // WorldPositionを計算して渡す
    output.worldPosition = mul(input.position, gTransformationMatrix.World).xyz;
    
    // 法線の変換 (回転のみ適用)
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.World));
    
    output.texcoord = input.texcoord;
    
    return output;
}