TextureCube<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float3 direction : TEXCOORD0;
};

float4 main(VertexShaderOutput input) : SV_TARGET
{
    return gTexture.Sample(gSampler, normalize(input.direction));
}
