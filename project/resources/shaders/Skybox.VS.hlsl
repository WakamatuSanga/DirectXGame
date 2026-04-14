struct TransformationMatrix
{
    float4x4 WVP;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct VertexShaderInput
{
    float4 position : POSITION0;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float3 direction : TEXCOORD0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    float4 clipPosition = mul(input.position, gTransformationMatrix.WVP);
    output.position = clipPosition.xyww;
    output.direction = input.position.xyz;

    return output;
}
