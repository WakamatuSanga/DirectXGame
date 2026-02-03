#include "Particle.hlsli"

// C++のRootParameter[1] (Texture) を受け取る
// register(t0) はピクセルシェーダーでのみ見えるように設定されているので、VSのt0と被ってもOK
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
	
	// テクスチャサンプリング
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
	
	// テクスチャの色 * 頂点シェーダーから来たパーティクルの色
    output.color = textureColor * input.color;
	
	// アルファテスト (完全に透明なら描画しない)
    if (output.color.a == 0.0)
    {
        discard;
    }
	
    return output;
}