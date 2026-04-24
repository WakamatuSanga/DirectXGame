struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
    float3 cameraPosition;
    float shininess;
};

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
    float alphaReference;
};

struct EnvironmentMapData
{
    int enableEnvironmentMap;
    float intensity;
    float2 padding;
};

struct DissolveData
{
    int enableDissolve;
    float threshold;
    float edgeWidth;
    float edgeGlowStrength;
    float edgeNoiseStrength;
    float3 padding;
    float4 edgeColor;
};

struct RandomNoiseData
{
    int enableRandom;
    int previewRandom;
    float intensity;
    float time;
};

struct RingAppearanceData
{
    int enableRingAppearance;
    int uvDirection;
    float innerRadiusRatio;
    float startAlpha;
    float endAlpha;
    float startFadeRange;
    float endFadeRange;
    float padding;
    float4 innerColor;
    float4 outerColor;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<EnvironmentMapData> gEnvironmentMapData : register(b2);
ConstantBuffer<DissolveData> gDissolveData : register(b3);
ConstantBuffer<RandomNoiseData> gRandomNoiseData : register(b4);
ConstantBuffer<RingAppearanceData> gRingAppearanceData : register(b5);

Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEnvironmentTexture : register(t1);
Texture2D<float4> gDissolveMaskTexture : register(t2);
SamplerState gSampler : register(s0);

struct VertexShaderOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
    float4 normal : SV_TARGET1;
};

float rand2dTo1d(float2 value)
{
    return frac(sin(dot(value, float2(12.9898f, 78.233f))) * 43758.5453f);
}

float2 GetRingLocalPosition(float2 uv)
{
    return float2((uv.x - 0.5f) * 2.0f, (0.5f - uv.y) * 2.0f);
}

float ComputeRingRadial(float2 uv)
{
    float radius = length(GetRingLocalPosition(uv));
    float innerRadiusRatio = saturate(gRingAppearanceData.innerRadiusRatio);
    return saturate((radius - innerRadiusRatio) / max(1.0f - innerRadiusRatio, 0.0001f));
}

float ComputeRingProgress(float2 uv)
{
    float2 local = GetRingLocalPosition(uv);
    return frac(atan2(local.y, local.x) / (2.0f * 3.14159265359f) + 1.0f);
}

float ComputeRingStartAlpha(float progress)
{
    float fadeRange = max(gRingAppearanceData.startFadeRange, 0.0001f);
    float blend = 1.0f - smoothstep(0.0f, fadeRange, progress);
    return lerp(1.0f, gRingAppearanceData.startAlpha, blend);
}

float ComputeRingEndAlpha(float progress)
{
    float fadeRange = max(gRingAppearanceData.endFadeRange, 0.0001f);
    float blend = smoothstep(1.0f - fadeRange, 1.0f, progress);
    return lerp(1.0f, gRingAppearanceData.endAlpha, blend);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    float4 transformedUV = mul(float4(input.uv, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 texColor = gTexture.Sample(gSampler, transformedUV.xy);
    if (gRingAppearanceData.enableRingAppearance != 0)
    {
        float radial = ComputeRingRadial(transformedUV.xy);
        float progress = ComputeRingProgress(transformedUV.xy);
        float2 ringSampleUV = (gRingAppearanceData.uvDirection == 0) ?
            float2(radial, 0.5f) :
            float2(0.5f, radial);
        float4 ringTint = lerp(gRingAppearanceData.innerColor, gRingAppearanceData.outerColor, radial);
        float alphaFactor = ComputeRingStartAlpha(progress) * ComputeRingEndAlpha(progress);
        texColor = gTexture.Sample(gSampler, ringSampleUV) * ringTint;
        texColor.a *= alphaFactor;
    }
    float dissolveEdge = 0.0f;
    if (gDissolveData.enableDissolve != 0)
    {
        float dissolveMask = gDissolveMaskTexture.Sample(gSampler, transformedUV.xy).r;
        clip(dissolveMask - gDissolveData.threshold);
        float edgeWidth = max(gDissolveData.edgeWidth, 0.0001f);
        dissolveEdge = 1.0f - smoothstep(gDissolveData.threshold, gDissolveData.threshold + edgeWidth, dissolveMask);
    }
    
    // Zバッファの不具合を防ぐため透明な部分は破棄
    float alphaDiscardThreshold = (gRingAppearanceData.enableRingAppearance != 0) ? 0.01f : gMaterial.alphaReference;
    if (texColor.a <= alphaDiscardThreshold)
    {
        discard;
    }
    
    float4 outputColor = gMaterial.color * texColor;

    if (gMaterial.enableLighting != 0)
    {
        float3 N = normalize(input.normal);
        float3 L = normalize(-gDirectionalLight.direction);
        
        // 拡散反射 (Lambert)
        float NdotL = saturate(dot(N, L));
        float3 diffuse = gDirectionalLight.color.rgb * NdotL * gDirectionalLight.intensity;
        
        // 鏡面反射 (Blinn-Phong)
        float3 V = normalize(gDirectionalLight.cameraPosition - input.worldPos);
        float3 H = normalize(L + V);
        float NdotH = saturate(dot(N, H));
        float specularWeight = pow(NdotH, gDirectionalLight.shininess);
        float3 specular = gDirectionalLight.color.rgb * specularWeight * gDirectionalLight.intensity;
        
        // 環境光 (Ambient)
        float3 ambient = float3(0.1f, 0.1f, 0.1f);
        
        // 合成
        outputColor.rgb = outputColor.rgb * (diffuse + ambient) + specular;
    }

    if (gEnvironmentMapData.enableEnvironmentMap != 0)
    {
        float3 N = normalize(input.normal);
        float3 V = normalize(gDirectionalLight.cameraPosition - input.worldPos);
        float3 reflection = reflect(-V, N);
        float3 reflectionColor = gEnvironmentTexture.Sample(gSampler, reflection).rgb;
        outputColor.rgb = lerp(outputColor.rgb, reflectionColor, saturate(gEnvironmentMapData.intensity));
    }

    if (gDissolveData.enableDissolve != 0)
    {
        float edgeNoise = rand2dTo1d(transformedUV.xy * 32.0f + gRandomNoiseData.time.xx);
        float edgeVariation = lerp(
            1.0f - gDissolveData.edgeNoiseStrength,
            1.0f + gDissolveData.edgeNoiseStrength,
            edgeNoise);
        float edgeGlow = dissolveEdge * edgeVariation;
        outputColor.rgb = lerp(outputColor.rgb, gDissolveData.edgeColor.rgb, saturate(edgeGlow * gDissolveData.edgeColor.a));
        outputColor.rgb += gDissolveData.edgeColor.rgb * edgeGlow * gDissolveData.edgeGlowStrength;
    }

    if (gRandomNoiseData.enableRandom != 0)
    {
        float random = rand2dTo1d(transformedUV.xy * 32.0f + gRandomNoiseData.time.xx);
        if (gRandomNoiseData.previewRandom != 0)
        {
            outputColor.rgb = random.xxx;
        }
        else
        {
            float noiseFactor = lerp(1.0f, random, saturate(gRandomNoiseData.intensity));
            outputColor.rgb *= noiseFactor;
        }
    }

    PixelShaderOutput output;
    output.color = outputColor;
    output.normal = float4(normalize(input.normal) * 0.5f + 0.5f, 1.0f);
    return output;
}
