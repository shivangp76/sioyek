#version 440
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv_in;

layout(location = 0) out vec2 uv;

layout(std140, binding = 0) uniform buf {
    vec4 current_color;
    float depth;
    float time;
    bool is_pending;
};

void main()
{
    uv = uv_in;
    gl_Position = vec4(position, depth, 1);
}
