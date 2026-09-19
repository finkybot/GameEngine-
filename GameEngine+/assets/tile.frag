#version 460 core

in vec2 uv;
in vec4 tint;

uniform sampler2D uTileAtlas;

out vec4 fragColor;

void main()
{
	vec4 texColor = texture(uTileAtlas, uv);
	fragColor = texColor * tint;
	//fragColor = vec4(1.0, 0.0, 0.0, 1.0); // Set the fragment color to red
}
