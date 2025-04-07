#version 440

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    vec4 current_color;
    float depth;
    float time;
    bool is_pending;
};


void main()
{
    if (is_pending) {
        float r = sin(time + uv.x * 1.0) * 0.5 + 0.5;
        float g = sin(time * 1.1 + uv.y * 1.0) * 0.5 + 0.5;
        float b = sin(time * 1.2 + uv.x * 1.0 + uv.y * 1.0) * 0.5 + 0.5;

        fragColor = vec4(r, g, b, current_color.a);
    }
    else{
        fragColor = current_color;
    }
}
