#version 440

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 position;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    vec2 offset;
    vec2 scale;
    float depth;
    float time;
    bool is_pending;
};

void main()
{
    if (is_pending){
        float r = (sin(position.x * 10) + 1) / 2;
        float g = (sin(position.y) * 10 + 1) / 2;
        fragColor = vec4(r, g, (sin(time) + 1) / 2, 1);
    }
    else{
        fragColor = color;
    }
}
