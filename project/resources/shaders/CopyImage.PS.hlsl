#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
Texture2D<float> gDepthTexture : register(t1);
SamplerState gSampler : register(s0);
ConstantBuffer<PostEffectParameters> gPostEffectParameters : register(b0);

float3 ApplySmoothing(float2 texcoord, float3 baseColor, float intensity)
{
    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 texelSize = 1.0f / float2(width, height);

    float3 accumulatedColor = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; ++y) {
        [unroll]
        for (int x = -1; x <= 1; ++x) {
            float2 offset = float2(x, y) * texelSize;
            accumulatedColor += gTexture.Sample(gSampler, texcoord + offset).rgb;
        }
    }

    float3 smoothedColor = accumulatedColor / 9.0f;
    return lerp(baseColor, smoothedColor, saturate(intensity));
}

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

float3 SampleOutlineSource(float2 texcoord)
{
    return gTexture.Sample(gSampler, texcoord).rgb;
}

float ApplyOutlineResponse(float edge)
{
    float threshold = gPostEffectParameters.outlineThreshold;
    float softness = max(gPostEffectParameters.outlineSoftness, 0.0001f);
    edge = smoothstep(threshold, threshold + softness, edge);
    return saturate(edge * gPostEffectParameters.outlineIntensity);
}

float ApplyDepthOutlineResponse(float edge)
{
    float threshold = gPostEffectParameters.outlineDepthThreshold;
    float softness = max(gPostEffectParameters.outlineSoftness, 0.0001f);
    edge = smoothstep(threshold, threshold + softness, edge);
    edge *= gPostEffectParameters.outlineDepthStrength;
    return saturate(edge * gPostEffectParameters.outlineIntensity);
}

float ComputeColorDiff8Edge(float2 texcoord, float2 thickness)
{
    float3 center = SampleOutlineSource(texcoord);
    float3 left = SampleOutlineSource(texcoord + float2(-thickness.x, 0.0f));
    float3 right = SampleOutlineSource(texcoord + float2(thickness.x, 0.0f));
    float3 up = SampleOutlineSource(texcoord + float2(0.0f, -thickness.y));
    float3 down = SampleOutlineSource(texcoord + float2(0.0f, thickness.y));
    float3 upLeft = SampleOutlineSource(texcoord + float2(-thickness.x, -thickness.y));
    float3 upRight = SampleOutlineSource(texcoord + float2(thickness.x, -thickness.y));
    float3 downLeft = SampleOutlineSource(texcoord + float2(-thickness.x, thickness.y));
    float3 downRight = SampleOutlineSource(texcoord + float2(thickness.x, thickness.y));

    float edge = 0.0f;
    edge = max(edge, length(center - left));
    edge = max(edge, length(center - right));
    edge = max(edge, length(center - up));
    edge = max(edge, length(center - down));
    edge = max(edge, length(center - upLeft));
    edge = max(edge, length(center - upRight));
    edge = max(edge, length(center - downLeft));
    edge = max(edge, length(center - downRight));

    return edge;
}

float SampleLuminance(float2 texcoord)
{
    return dot(SampleOutlineSource(texcoord), float3(0.2125f, 0.7154f, 0.0721f));
}

float ComputeSobelEdge(float2 texcoord, float2 thickness)
{
    float topLeft = SampleLuminance(texcoord + float2(-thickness.x, -thickness.y));
    float top = SampleLuminance(texcoord + float2(0.0f, -thickness.y));
    float topRight = SampleLuminance(texcoord + float2(thickness.x, -thickness.y));
    float left = SampleLuminance(texcoord + float2(-thickness.x, 0.0f));
    float right = SampleLuminance(texcoord + float2(thickness.x, 0.0f));
    float bottomLeft = SampleLuminance(texcoord + float2(-thickness.x, thickness.y));
    float bottom = SampleLuminance(texcoord + float2(0.0f, thickness.y));
    float bottomRight = SampleLuminance(texcoord + float2(thickness.x, thickness.y));

    float gx = (-topLeft) + topRight
        + (-2.0f * left) + (2.0f * right)
        + (-bottomLeft) + bottomRight;

    float gy = (-topLeft) + (-2.0f * top) + (-topRight)
        + bottomLeft + (2.0f * bottom) + bottomRight;

    return length(float2(gx, gy));
}

float SampleDepth(float2 texcoord)
{
    return gDepthTexture.Sample(gSampler, texcoord);
}

float ComputeDepthEdge(float2 texcoord, float2 thickness)
{
    float centerDepth = SampleDepth(texcoord);
    float leftDepth = SampleDepth(texcoord + float2(-thickness.x, 0.0f));
    float rightDepth = SampleDepth(texcoord + float2(thickness.x, 0.0f));
    float upDepth = SampleDepth(texcoord + float2(0.0f, -thickness.y));
    float downDepth = SampleDepth(texcoord + float2(0.0f, thickness.y));

    float edge = 0.0f;
    edge = max(edge, abs(centerDepth - leftDepth));
    edge = max(edge, abs(centerDepth - rightDepth));
    edge = max(edge, abs(centerDepth - upDepth));
    edge = max(edge, abs(centerDepth - downDepth));

    return edge;
}

float ComputeHybridEdge(float2 texcoord, float2 thickness)
{
    float edgeColor = 0.0f;
    if (gPostEffectParameters.hybridColorSource == 1) {
        edgeColor = ComputeColorDiff8Edge(texcoord, thickness);
    } else {
        edgeColor = ComputeSobelEdge(texcoord, thickness);
    }

    float edgeDepth = ComputeDepthEdge(texcoord, thickness);

    edgeColor = ApplyOutlineResponse(edgeColor);
    edgeDepth = ApplyDepthOutlineResponse(edgeDepth);

    return saturate(
        edgeColor * gPostEffectParameters.hybridColorWeight +
        edgeDepth * gPostEffectParameters.hybridDepthWeight);
}

float3 ApplyOutline(float2 texcoord, float3 baseColor)
{
    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 texelSize = 1.0f / float2(width, height);
    float2 thickness = texelSize * max(gPostEffectParameters.outlineThickness, 0.5f);

    float edge = 0.0f;
    if (gPostEffectParameters.outlineMode == 1) {
        edge = ComputeColorDiff8Edge(texcoord, thickness);
        edge = ApplyOutlineResponse(edge);
    } else if (gPostEffectParameters.outlineMode == 2) {
        edge = ComputeSobelEdge(texcoord, thickness);
        edge = ApplyOutlineResponse(edge);
    } else if (gPostEffectParameters.outlineMode == 3) {
        edge = ComputeDepthEdge(texcoord, thickness);
        edge = ApplyDepthOutlineResponse(edge);
    } else if (gPostEffectParameters.outlineMode == 4) {
        edge = ComputeHybridEdge(texcoord, thickness);
    }

    return lerp(baseColor, gPostEffectParameters.outlineColor.rgb, edge);
}

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float4 sampledColor = gTexture.Sample(gSampler, input.texcoord);
    float3 color = sampledColor.rgb;

    float smoothingIntensity = gPostEffectParameters.smoothingIntensity * gPostEffectParameters.smoothingEnabled;
    float grayscaleIntensity = gPostEffectParameters.grayscaleIntensity * gPostEffectParameters.grayscaleEnabled;
    float sepiaIntensity = gPostEffectParameters.sepiaIntensity * gPostEffectParameters.sepiaEnabled;
    float invertIntensity = gPostEffectParameters.invertIntensity * gPostEffectParameters.invertEnabled;
    float vignetteIntensity = gPostEffectParameters.vignetteIntensity * gPostEffectParameters.vignetteEnabled;

    color = ApplySmoothing(input.texcoord, color, smoothingIntensity);
    color = ApplyGrayscale(color, grayscaleIntensity);
    color = ApplySepia(color, sepiaIntensity);
    color = ApplyInvert(color, invertIntensity);
    color *= ApplyVignette(input.texcoord, vignetteIntensity);
    if (gPostEffectParameters.outlineMode != 0) {
        color = ApplyOutline(input.texcoord, color);
    }

    return float4(color, sampledColor.a);
}
