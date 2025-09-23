#include <simd/simd.h>

typedef uint32_t uint32;
typedef float real32;

// Note: (Ted)  These are the vertex shader input data on the Mac Platform.
typedef struct
{
    vector_float2 position;
    vector_float2 textureCoordinate;
    uint32 textureID;
    real32 alpha;

} MacTextureShaderVertex;

typedef struct
{
    real32 Width;
    real32 Height; 
} MacTextureSize;

typedef enum TextureIndex
{
    TextureIndexBaseColor = 0,
} TextureIndex;
