#version 440
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv_in;

layout(location = 0) out vec2 uv;

layout(std140, binding = 1) uniform buf {
    float depth;
};

void main()
{
    uv = uv_in;
    gl_Position = vec4(position, depth, 1);
}
