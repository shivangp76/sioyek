#version 440
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec2 out_position;

layout(std140, binding = 0) uniform buf {
    vec2 offset;
    vec2 scale;
    float depth;
    float time;
    bool is_pending;
};

void main()
{
    out_color = color;
    vec2 transformed_position = position * scale + offset;

    out_position = position;
    gl_Position = vec4(transformed_position, depth, 1);
    // gl_Position = vec4(position, 0, 1);
}
