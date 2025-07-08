#version 330

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat4  modelview, projection;
uniform float particle_scale;

in int id[1];

out vec2 coord;

void emit(vec2 p)
{
    coord = p;
    p *= (0.006 + 0.003 * cos(float(id[0]))) * 0.6 * particle_scale + (((id[0] & 31) == 0) ? 0.06 : 0.0);
    vec3 vert = gl_in[0].gl_Position.xyz + p.x * inverse(modelview)[0].xyz + p.y * inverse(modelview)[1].xyz;
    gl_Position = projection * modelview * vec4(vert, 1.0);
    EmitVertex();
}

void main()
{
    emit(vec2(+1.0, -1.0));
    emit(vec2(+1.0, +1.0));
    emit(vec2(-1.0, -1.0));
    emit(vec2(-1.0, +1.0));
    EndPrimitive();
}

