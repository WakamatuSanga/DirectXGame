#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<PostEffectParameters> gPostEffectParameters : register(b0);

float3 ApplyGrayscale(float3 color, float intensity)
{
    float luminance = dot(color, float3(0.2125f, 0.7154f, 0.0721f));
    return lerp(color, luminance.xxx, saturate(intensity));
}

float3 ApplySepia(float3 color, float intensity)
{
    float3 sepiaColor;
    sepiaColor.r = dot(color, float3(0.393f, 0.769f, 0.189f));
    sepiaColor.g = dot(color, float3(0.349f, 0.686f, 0.168f));
    sepiaColor.b = dot(color, float3(0.272f, 0.534f, 0.131f));
    return lerp(color, saturate(sepiaColor), saturate(intensity));
}

float3 ApplyInvert(float3 color, float intensity)
{
    return lerp(color, 1.0f - color, saturate(intensity));
}

float ApplyVignette(float2 texcoord, float intensity)
{
    float2 centered = texcoord - 0.5f;
    float distanceFromCenter = length(centered) * 1.41421356f;
    float vignette = saturate(1.0f - distanceFromCenter * distanceFromCenter);
    vignette = pow(vignette, 1.5f);
    return lerp(1.0f, vignette, saturate(intensity));
}

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float4 sampledColor = gTexture.Sample(gSampler, input.texcoord);
    float3 color = sampledColor.rgb;

    float grayscaleIntensity = gPostEffectParameters.grayscaleIntensity * gPostEffectParameters.grayscaleEnabled;
    float sepiaIntensity = gPostEffectParameters.sepiaIntensity * gPostEffectParameters.sepiaEnabled;
    float invertIntensity = gPostEffectParameters.invertIntensity * gPostEffectParameters.invertEnabled;
    float vignetteIntensity = gPostEffectParameters.vignetteIntensity * gPostEffectParameters.vignetteEnabled;

    color = ApplyGrayscale(color, grayscaleIntensity);
    color = ApplySepia(color, sepiaIntensity);
    color = ApplyInvert(color, invertIntensity);
    color *= ApplyVignette(input.texcoord, vignetteIntensity);

    return float4(color, sampledColor.a);
}
