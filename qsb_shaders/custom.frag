#version 440

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;
layout(binding = 0) uniform sampler2D tex;

layout(std140, binding = 1) uniform buf {
    mat4 transform_matrix;
    float depth;
    float contrast;
};

void main(){
    vec4 pdf_color = vec4(texture(tex, uv).rgb, 1);
    vec3 resulting_unclamped_color = (transform_matrix * pdf_color).rgb;
    fragColor = vec4(clamp(resulting_unclamped_color, vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0)), 1.0);
}
