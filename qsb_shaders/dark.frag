#version 440

layout(location = 0) in vec2 uvs;

layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform sampler2D tex;

layout(std140, binding = 1) uniform buf {
    mat4 transform_matrix;
    float depth;
    float contrast;
};

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y); float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}


void main(){
    vec3 tempcolor = texture(tex, uvs).rgb;
    vec3 inv = (0.5-tempcolor)*contrast+0.5; //Invert colors and shift colors from range 0.0 - 1.0 to -0.5 - 0.5, apply contrast and shift back to 0.0 - 1.0. This way contrast applies on both whites and blacks
    vec3 hsvcolor = rgb2hsv(inv); //transform to hsv
    float new_hue = mod(hsvcolor.r + 0.5, 1.0); // shift hue 180 degrees to compensate hue shift from inverting colors 
    vec3 newcolor = hsv2rgb(vec3(new_hue,hsvcolor.gb)); 
    fragColor = vec4(newcolor, 1.0);
}
