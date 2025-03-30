#version 440

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    vec4 current_color;
    float depth;
};


void main()
{
    fragColor = current_color;
}
