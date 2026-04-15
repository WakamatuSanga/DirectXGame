#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<PostEffectParameters> gPostEffectParameters : register(b0);

float4 main(VertexShaderOutput input) : SV_TARGET
{
    uint width, height;
    gTexture.GetDimensions(width, height);

    float radius = gPostEffectParameters.gaussianIntensity;
    float2 texelOffset = float2(1.0f / width, 0.0f) * radius;

    float4 color = gTexture.Sample(gSampler, input.texcoord) * 0.375f;
    color += gTexture.Sample(gSampler, input.texcoord + texelOffset) * 0.25f;
    color += gTexture.Sample(gSampler, input.texcoord - texelOffset) * 0.25f;
    color += gTexture.Sample(gSampler, input.texcoord + texelOffset * 2.0f) * 0.0625f;
    color += gTexture.Sample(gSampler, input.texcoord - texelOffset * 2.0f) * 0.0625f;

    return color;
}
