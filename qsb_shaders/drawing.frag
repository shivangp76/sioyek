#version 440

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 position;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    vec2 offset;
    vec2 scale;
    float depth;
    float time;
};

void main()
{
    if (color.a < 0){
        float r = sin(time + position.x * 10.0) * 0.5 + 0.5;
        float g = sin(time * 1.1 + position.y * 10.0) * 0.5 + 0.5;
        float b = sin(time * 1.2 + position.x * 10.0 + position.y * 10.0) * 0.5 + 0.5;
        fragColor = vec4(r, g, b, 1);
    }
    else{
        fragColor = color;
    }
}
