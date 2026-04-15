struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

struct PostEffectParameters
{
    uint grayscaleEnabled;
    float grayscaleIntensity;
    uint sepiaEnabled;
    float sepiaIntensity;
    uint invertEnabled;
    float invertIntensity;
    uint vignetteEnabled;
    float vignetteIntensity;
};
