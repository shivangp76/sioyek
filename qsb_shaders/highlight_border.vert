#version 440
layout(location = 0) in vec2 position;

layout(std140, binding = 0) uniform buf {
    vec4 current_color;
    float depth;
};

void main()
{
    gl_Position = vec4(position, depth, 1);
}
