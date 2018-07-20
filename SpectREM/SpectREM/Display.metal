//
//  Display.metal
//  SpectREM
//
//  Created by Mike on 20/07/2018.
//  Copyright © 2018 71Squared Ltd. All rights reserved.
//

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

#import "ShaderTypes.h"

// Vertex shader outputs and per-fragment inputs. Includes clip-space position and vertex outputs
//  interpolated by rasterizer and fed to each fragment generated by clip-space primitives.
typedef struct
{
    float4 clipSpacePosition [[position]];
    float2 textureCoordinate;
    
} RasterizerData;

// Vertex Function
vertex RasterizerData
vertexShader(uint vertexID [[ vertex_id ]],
             constant Vertex *vertexArray [[ buffer(VertexInputIndexVertices) ]],
             constant vector_uint2 *viewportSizePointer  [[ buffer(VertexInputIndexViewportSize) ]])

{
    RasterizerData out;
    float2 pixelSpacePosition = vertexArray[vertexID].position.xy;
    out.clipSpacePosition.xy = pixelSpacePosition;
    out.clipSpacePosition.z = 0.0;
    out.clipSpacePosition.w = 1.0;
    out.textureCoordinate = vertexArray[vertexID].textureCoordinate;
    
    return out;
}

// Fragment function
fragment float4
samplingShader(RasterizerData in [[stage_in]],
               texture2d<half> colorTexture [[ texture(TextureIndexBaseColor) ]],
               texture1d<float> clutTexture [[ texture(TextureCLUTColor) ]])
{
    constexpr sampler textureSampler (mag_filter::nearest,
                                      min_filter::nearest);
    
    const float clutUVAdjust = 1.0 / 16.0;

    const int colorSample = colorTexture.sample(textureSampler, in.textureCoordinate)[0] * 256;
    const float4 displayColor = clutTexture.sample(textureSampler, colorSample * clutUVAdjust);

    // We return the color of the texture
    return float4(displayColor);
}

