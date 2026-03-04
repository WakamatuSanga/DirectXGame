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
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct VertexShaderOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD1;
};

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float4 transformedUV = mul(float4(input.uv, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 texColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    // Zバッファの不具合を防ぐため透明な部分は破棄
    if (texColor.a <= 0.5f)
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

    return outputColor;
}