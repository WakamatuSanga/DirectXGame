// Particle.VS.hlsl
struct VSIn
{
    float32_t4 pos : POSITION;
    float32_t2 uv : TEXCOORD;
};

struct VSOut
{
    float32_t4 svpos : SV_POSITION;
    float32_t2 uv : TEXCOORD0;
};

struct TransformationMatrix
{
    float32_t4x4 WVP;
    float32_t4x4 World;
};

// C++ 側 RootParam[1] : VS の t0（StructuredBuffer）
StructuredBuffer<TransformationMatrix> gInstances : register(t0);

VSOut main(VSIn vin, uint iid : SV_InstanceID)
{
    VSOut o;
    o.svpos = mul(vin.pos, gInstances[iid].WVP); // ← 個別行列を適用
    o.uv = vin.uv;
    return o;
}
