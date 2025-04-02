#version 440
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 out_color;

layout(std140, binding = 0) uniform buf {
    vec2 offset;
    float scale;
};

void main()
{
    out_color = color;
    vec2 transformed_position = position * scale + offset;

    gl_Position = vec4(transformed_position, 0, 1);
    // gl_Position = vec4(position, 0, 1);
}
