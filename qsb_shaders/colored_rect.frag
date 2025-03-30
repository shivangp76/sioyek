#version 440

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform sampler2D tex;

void main()
{
    // fragColor = vec4(rect_color, 1);
    // fragColor = vec4(uv, 1, 1);
    fragColor = texture(tex, uv);
    // fragColor = vec4(1, 0, 0, 1);
    // fragColor = vec4(uv, 0, 0);
}
