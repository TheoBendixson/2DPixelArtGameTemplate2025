
#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

#define MAX_TEXTURES 200 

#import "CommonShaderTypes.h"
#import "TextureShaderTypes.h"

typedef uint32_t u32;

typedef enum PixelArtShaderVSAttribute
{
    PixelArtShaderVSAttributePosition = 0,
    PixelArtShaderVSAttributeUV = 1
} PixelArtShaderVSAttribute;

typedef struct
{
    float2 position [[attribute(PixelArtShaderVSAttributePosition)]];
    float2 uv       [[attribute(PixelArtShaderVSAttributeUV)]];
} PixelArtShaderVSInput;

typedef struct
{
    packed_float2 vMin;
    packed_float2 vMax;
    float         rotation;
    u32           textureID;
    float         alpha;
} PixelArtShaderInstanceUniforms;

typedef struct
{
    float4 Position [[position]];
    float2 TextureCoordinate;
    
    float2 vUv;
    float2 TextureSize;

    uint32_t TextureID;
    float Alpha;

} PixelArtShaderRasterizerData;

vertex PixelArtShaderRasterizerData
pixelArtVertexShader(uint vertexID [[ vertex_id ]],
                     uint instanceID [[ instance_id ]],
                     constant PixelArtShaderVSInput *vertexArray [[ buffer(BufferIndexVertices) ]],
                     constant vector_uint2 *viewportSizePointer  [[ buffer(BufferIndexViewportSize) ]],
                     constant MacTextureSize *textureSize [[buffer(BufferIndexTextureSize) ]],
                     constant PixelArtShaderInstanceUniforms *perInstanceUniforms [[ buffer(BufferIndexPerInstanceUniforms) ]])
{
    PixelArtShaderRasterizerData out;

    PixelArtShaderVSInput in = vertexArray[vertexID];
    PixelArtShaderInstanceUniforms instanceUniforms = perInstanceUniforms[instanceID];

    // Get the viewport size and cast to float.
    float2 viewportSize = float2(*viewportSizePointer);

    // Scale: map [-0.5, 0.5] quad verts to pixel-space dimensions
    float2 scale = instanceUniforms.vMax - instanceUniforms.vMin;
    float2 scaled = in.position * scale;

    // Rotate
    float c = cos(instanceUniforms.rotation);
    float s = sin(instanceUniforms.rotation);
    float2 rotated = float2(c * scaled.x + s * scaled.y,
                           -s * scaled.x + c * scaled.y);

    // Translate to quad center in Y-flipped screen space
    float2 center = float2(instanceUniforms.vMin.x + 0.5 * scale.x,
                           (viewportSize.y - instanceUniforms.vMin.y) - 0.5 * scale.y);
    float2 worldPos = rotated + center;

    // NDC conversion
    out.Position = vector_float4(0.0, 0.0, 0.0, 1.0);
    out.Position.xy = (worldPos / (viewportSize / 2.0)) - 1.0;

    out.TextureCoordinate = in.uv;
    out.TextureSize = float2(textureSize->Width, textureSize->Height);
    out.vUv = out.TextureSize*out.TextureCoordinate;
    out.TextureID = instanceUniforms.textureID;
    out.Alpha = instanceUniforms.alpha;
    return out;
}

// Fragment functions
fragment float4
pixelArtFragmentShader(PixelArtShaderRasterizerData in [[stage_in]],
                       texture2d_array<half> texture_atlas [[ texture(0) ]])
{
    constexpr sampler textureSampler (mag_filter::linear,
                                      min_filter::linear);

    float2 Pixel = in.vUv;
    float2 Seam = floor(Pixel + 0.5);
    float2 DuDv = fwidth(Pixel);
    Pixel = Seam + clamp( (Pixel - Seam)/DuDv, -0.5, 0.5);
    float2 ModifiedTextureCoordinate = Pixel/in.TextureSize;

    const half4 colorSample = texture_atlas.sample(textureSampler, ModifiedTextureCoordinate, in.TextureID);

    // NOTE: (Ted)  Multiply by global alpha since alpha is already normalized.
    float4 Tex = float4(colorSample[0], colorSample[1], colorSample[2], colorSample[3]);

    if (Tex.a < .95)
    {
        discard_fragment();
    }

    Tex = float4(Tex[0], Tex[1], Tex[2], in.Alpha*Tex[3]);
    return Tex;
}
