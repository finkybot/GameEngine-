#version 460 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec4 inUV;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec4 inWorld;
layout(location = 4) in uvec2 inHandle;   // <-- bindless handle

uniform vec2 uViewportSize;

out vec2 uv;
out vec4 tint;
flat out uvec2 texHandle;                      // <-- pass to fragment shader

void main()
{
    float x = inWorld.x + inPos.x * inWorld.z;
    float y = inWorld.y + inPos.y * inWorld.w;

    float ndcX = (x / uViewportSize.x) * 2.0 - 1.0;
    float ndcY = 1.0 - (y / uViewportSize.y) * 2.0;

    gl_Position = vec4(ndcX, ndcY, 0.0, 1.0);

    uv = mix(inUV.xy, inUV.zw, inPos);
    tint = inColor;
    texHandle = inHandle;                 // <-- forward bindless handle
}

