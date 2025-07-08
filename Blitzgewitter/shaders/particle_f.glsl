#version 330

in vec2 coord;

out vec4 output_colour;

void main()
{
    output_colour.rgb = vec3(0.3, 0.7, 1.0).bgr * 0.1 * (1.0 - smoothstep(0.9, 1.0, length(coord))) * 4.0;
    output_colour.a = 0.0;
}
