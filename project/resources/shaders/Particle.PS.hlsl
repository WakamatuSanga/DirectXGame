#include "Particle.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    float32_t3 _pad; // 16byte アライン用
    float32_t4x4 uvTransform;
};

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
    float32_t4 color : SV_Target0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // UV変換
    float32_t4 uv4 = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t2 uv = uv4.xy;

    float32_t4 tex = gTexture.Sample(gSampler, uv);

    output.color = gMaterial.color * tex;

    // 完全透明は捨てる（discard ではなく clip の方が確実）
    clip(output.color.a - 1e-6);

    return output;
}
