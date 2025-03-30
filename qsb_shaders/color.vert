
#version 440
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv_coords;

layout(location = 0) out vec2 uv;

// layout(location = 1) in vec3 color;
// layout(location = 0) out vec3 v_color;
// layout(std140, binding = 0) uniform buf {
//     mat4 mvp;
// };

// layout(std140, binding = 0) uniform buf {
    //vec4 current_color;
//};

void main()
{
    // v_color = color;

    uv = uv_coords;
    gl_Position = vec4(position, 0, 1);

    // gl_Position = mvp * position;
}
