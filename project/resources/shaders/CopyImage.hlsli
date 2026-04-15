struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

struct PostEffectParameters
{
    uint gaussianEnabled;
    float gaussianIntensity;
    uint smoothingEnabled;
    float smoothingIntensity;
    uint grayscaleEnabled;
    float grayscaleIntensity;
    uint sepiaEnabled;
    float sepiaIntensity;
    uint invertEnabled;
    float invertIntensity;
    uint vignetteEnabled;
    float vignetteIntensity;
    uint outlineMode;
    float outlineIntensity;
    float outlineThickness;
    float outlineThreshold;
    float outlineSoftness;
    float outlineDepthThreshold;
    float outlineDepthStrength;
    uint hybridColorSource;
    float hybridColorWeight;
    float hybridDepthWeight;
    float2 outlinePadding;
    float4 outlineColor;
};
