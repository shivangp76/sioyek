#version 440
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    vec3 current_color;
};

layout(binding = 1) uniform sampler2D tex;

void main()
{
    // use the texture
    fragColor = texture(tex, uv);
}
