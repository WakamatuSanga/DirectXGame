#include "Object3d.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
    float shininess;
    int lightingType; // 0:Half, 1:Lambert, 2:Phong, 3:Blinn
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

struct Camera
{
    float3 worldPosition;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    if (gMaterial.enableLighting != 0)
    {
        float3 N = normalize(input.normal);
        float3 L = normalize(-gDirectionalLight.direction);
        
        float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
        
        float diffuseWeight = 0.0f;
        float specularWeight = 0.0f;
        float NdotL = dot(N, L);

        // --- ライティングタイプ ---
        if (gMaterial.lightingType == 0)
        { // Half-Lambert
            diffuseWeight = pow(NdotL * 0.5f + 0.5f, 2.0f);
            specularWeight = 0.0f;
        }
        else if (gMaterial.lightingType == 1)
        { // Lambert
            diffuseWeight = saturate(NdotL);
            specularWeight = 0.0f;
        }
        else if (gMaterial.lightingType == 2)
        { // Phong
            diffuseWeight = saturate(NdotL);
            float3 R = reflect(-L, N);
          
            float RdotV = dot(R, toEye); 
            specularWeight = pow(saturate(RdotV), gMaterial.shininess);
        }
        else if (gMaterial.lightingType == 3)
        { // Blinn-Phong
            diffuseWeight = saturate(NdotL);
            
            float3 H = normalize(L + toEye); 
            float NdotH = dot(N, H);
            specularWeight = pow(saturate(NdotH), gMaterial.shininess);
        }
        else if (gMaterial.lightingType == 4)
        { // Toon
            diffuseWeight = saturate(NdotL);
            float3 R = reflect(-L, N);
            
            float RdotV = dot(R, toEye);
            float spec = pow(saturate(RdotV), gMaterial.shininess);
            specularWeight = step(0.5f, spec);
        }

        float3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * diffuseWeight * gDirectionalLight.intensity;
        
        if (NdotL <= 0.0f && gMaterial.lightingType != 0)
        {
            specularWeight = 0.0f;
        }
        
        float3 specular = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularWeight;
        
        output.color.rgb = diffuse + specular;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    
    return output;
}