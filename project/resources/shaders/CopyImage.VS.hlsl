#include "CopyImage.hlsli"

VertexShaderOutput main(uint vertexID : SV_VertexID)
{
    static const float2 kPositions[3] = {
        float2(-1.0f, 1.0f),
        float2(3.0f, 1.0f),
        float2(-1.0f, -3.0f),
    };

    static const float2 kTexcoords[3] = {
        float2(0.0f, 0.0f),
        float2(2.0f, 0.0f),
        float2(0.0f, 2.0f),
    };

    VertexShaderOutput output;
    output.position = float4(kPositions[vertexID], 0.0f, 1.0f);
    output.texcoord = kTexcoords[vertexID];
    return output;
}
