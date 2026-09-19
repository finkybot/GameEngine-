#version 460 core
#extension GL_ARB_bindless_texture : require

in vec2 uv;
in vec4 tint;
flat in uvec2 texHandle;        // <-- from vertex shader

out vec4 fragColor;

void main()
{
    // Construct a bindless sampler from the 64‑bit handle
    sampler2D atlas = sampler2D(texHandle);

    // Sample normally
    vec4 texColor = texture(atlas, uv);

    fragColor = texColor * tint;
}
