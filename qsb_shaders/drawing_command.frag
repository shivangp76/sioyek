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
    if (color.a < 0){
        fragColor = vec4(1, 0, 0, 0.5);
    }
    else{
        fragColor = vec4(0, 0, 0, 0.0);
    }
}
