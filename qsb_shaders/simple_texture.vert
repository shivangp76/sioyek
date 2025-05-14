#version 440
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv_in;

layout(location = 0) out vec2 uv;

layout(std140, binding = 1) uniform buf {
    mat4 transform_matrix;
    float depth;
    float contrast;
};

void main()
{
    uv = uv_in;
    gl_Position = vec4(position, depth, 1);
}
