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

///////////////////////////////////////////////////////////////////////////////////////
// Distortion used to give the screen a curved effect
///////////////////////////////////////////////////////////////////////////////////////
vector_float2 radialDistortion(vector_float2 pos, float distortion)
{
    vector_float2 cc = pos - vector_float2(0.5, 0.5);
    float dist = dot(cc, cc) * distortion;
    return (pos + cc * (0.5 + dist) * dist);
}

///////////////////////////////////////////////////////////////////////////////////////
// colorCorrection used to adjust the saturation, constrast and brightness of the image
///////////////////////////////////////////////////////////////////////////////////////
vector_float3 colorCorrection(vector_float3 color, float saturation, float contrast, float brightness)
{
    const vector_float3 meanLuminosity = vector_float3(0.5, 0.5, 0.5);
    const vector_float3 rgb2greyCoeff = vector_float3(0.2126, 0.7152, 0.0722);    // Updated greyscal coefficients for sRGB and modern TVs
    
    vector_float3 brightened = color * brightness;
    float intensity = dot(brightened, rgb2greyCoeff);
    vector_float3 saturated = mix(vector_float3(intensity), brightened, saturation);
    vector_float3 contrasted = mix(meanLuminosity, saturated, contrast);
    
    return contrasted;
}

///////////////////////////////////////////////////////////////////////////////////////
// Split the red, green and blue channels of the texture passed in
///////////////////////////////////////////////////////////////////////////////////////
vector_float3 channelSplit(texture2d<half>image, sampler tex, vector_float2 coord, float spread){
    vector_float3 frag;
    frag.r = image.sample(tex, vector_float2(coord.x - spread, coord.y)).r;
    frag.g = image.sample(tex, vector_float2(coord.x, coord.y)).g;
    frag.b = image.sample(tex, vector_float2(coord.x + spread, coord.y)).b;
    return frag;
}

///////////////////////////// Vertex Function
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

///////////////////////////// Fragment function

fragment float4 samplingShader(RasterizerData in [[stage_in]],
                               texture2d<half> colorTexture [[ texture(TextureIndexBaseColor) ]],
                               texture1d<float> clutTexture [[ texture(TextureCLUTColor) ]],
                               constant Uniforms *uniforms [[buffer(TextureUniforms) ]])
{
    constexpr sampler textureSampler (mag_filter::nearest,
                                      min_filter::nearest);
    
//    float max = pow(u_vignetteX, u_vignetteY);
    const float w = 32 + 256 + 32;
    const float h = 32 + 192 + 32;
    const float clutUVAdjust = 1.0 / 16.0;
    float border = 32 - uniforms->displayBorderSize;
    float new_w = w - (border * 2);
    float new_h = h - (border * 2);
    float4 color;
    
    vector_float2 texCoord = radialDistortion(in.textureCoordinate, uniforms->displayCurvature);
    
    // Anything outside the texture should be black, otherwise sample the texel in the texture
    if (texCoord.x < 0 || texCoord.y < 0 || texCoord.x > 1 || texCoord.y > 1)
    {
        color = vector_float4(0, 0, 0, 1);
    }
    else
    {
        // Update the UV coordinates based on the size of the border
        float u = ((texCoord.x * new_w) + border);
        float v = ((texCoord.y * new_h) - border);
        
        // Apply pixel filtering
        float2 vUv = vector_float2(u, v);
        float alpha = float(uniforms->displayPixelFilterValue); // 0.5 = Linear, 0.0 = Nearest
        float2 x = fract(vUv);
        float2 x_ = clamp(0.5 / alpha * x, 0.0, 0.5) + clamp(0.5 / alpha * (x - 1.0) + 0.5, 0.0, 0.5);
        texCoord = (floor(vUv) + x_) / float2(w, h);
        
        const int colorSample = colorTexture.sample(textureSampler, texCoord)[0] * 256;
        color = clutTexture.sample(textureSampler, colorSample * clutUVAdjust);
    }

    // We return the color of the texture
    return float4(color);
}
