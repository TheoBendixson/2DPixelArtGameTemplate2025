
struct VS_INPUT                                                                
{                                                                              
    float2 pos      : POSITION;       
    float2 uv       : TEXCOORD;                                                 
};

struct PS_INPUT                                                                
{                                                                              
    float4 pos      : SV_POSITION;   
    float2 uv       : TEXCOORD;                                                 
    uint textureID  : TEXTUREID;                                                
    float alpha     : ALPHA;                                                    
};                                                                             

struct VS_INSTANCE
{
    float3 transformRow1    : TXROWONE;
    float3 transformRow2    : TXROWTWO;
    float3 transformRow3    : TXROWTHREE;
    uint textureID          : TEXTUREID;
    float alpha             : ALPHA;
};

cbuffer cbuffer0 : register(b0)      // b0 = constant buffer bound to slot 0
{                                                                              
    float2 ViewportSize;                                                       
}                                                                              

cbuffer cbuffer1 
{
    float2 TextureSize;                                                       
}

sampler sampler0 : register(s0);                
Texture2DArray<float4> texture0 : register(t0);

PS_INPUT vs(VS_INPUT input, VS_INSTANCE instance)                                                    
{
    PS_INPUT output;
    float3x3 transformMatrix = float3x3(instance.transformRow1, 
                                        instance.transformRow2, 
                                        instance.transformRow3);
    float3 positionVector = float3(input.pos, 1.0);
    float3 transformedPosition = mul(positionVector, transformMatrix);
    output.pos = float4(transformedPosition.xy, 0.f, 1.f);                               
    output.pos.xy = (output.pos.xy / (ViewportSize / 2.0)) - 1;
    output.uv = input.uv;
    output.textureID = instance.textureID;
    output.alpha = instance.alpha;
    return output;
}

float4 ps(PS_INPUT input) : SV_TARGET                                          
{                                                                              
    float2 Pixel = input.uv*TextureSize;
    float2 Seam = floor(Pixel + 0.5);
    float2 DuDv = fwidth(Pixel);
    Pixel = Seam + clamp((Pixel - Seam)/DuDv, -0.5, 0.5);
    float2 ModifiedTextureCoordinate = Pixel/TextureSize;
    float4 tex = texture0.Sample(sampler0, float3(ModifiedTextureCoordinate, input.textureID)); 

    if (tex.a < .95)
    {
        discard;
    }

    tex = float4(tex[0], tex[1], tex[2], input.alpha*tex[3]);
    return tex;                                                                
}                                                                              

