#version 330

layout(lines) in;
layout(triangle_strip, max_vertices = 8) out;

uniform vec2 radii;
uniform float aspect_ratio;

in vec3 v2g_c[2];

noperspective out vec3 lcoord;
noperspective out float zcoord;
out vec3 g2f_c;

void emit(vec2 p, vec2 c, float t, float z, vec3 col)
{
    lcoord.xy = c;
    lcoord.z = t;
    zcoord = z;
    g2f_c = col;
    gl_Position.xy = p;
    gl_Position.zw = vec2(0.0, 1.0);
    EmitVertex();
}

void main()
{
    vec2 pos0 = gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
    vec2 pos1 = gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;

    float z0 = gl_in[0].gl_Position.z / gl_in[0].gl_Position.w;
    float z1 = gl_in[1].gl_Position.z / gl_in[1].gl_Position.w;
    
    float rad_ratio = radii.y / radii.x;
    
    vec2 pll = normalize(pos1 - pos0) * radii.x, // parallel
         prp = pll.yx * vec2(-1, +1); // perpendicular

    pll.x*=aspect_ratio;
    prp.x*=aspect_ratio;
         
    emit(pos0 + prp * (+1.0) + pll * (-1.0), vec2(+1.0, -1.0), -1.0, z0, v2g_c[0]);
    emit(pos0 + prp * (-1.0) + pll * (-1.0), vec2(-1.0, -1.0), -1.0, z0, v2g_c[0]);

    emit(pos0 + prp * (+1.0), vec2(+1.0, 0.0), -1.0, z0, v2g_c[0]);
    emit(pos0 + prp * (-1.0), vec2(-1.0, 0.0), -1.0, z0, v2g_c[0]);

    emit(pos1 + prp * (+1.0) * rad_ratio, vec2(+1.0, 0.0), +1.0, z1, v2g_c[1]);
    emit(pos1 + prp * (-1.0) * rad_ratio, vec2(-1.0, 0.0), +1.0, z1, v2g_c[1]);
 
    emit(pos1 + prp * (+1.0) * rad_ratio + pll * (+1.0) * rad_ratio, vec2(+1.0, +1.0), +1.0, z1, v2g_c[1]);
    emit(pos1 + prp * (-1.0) * rad_ratio + pll * (+1.0) * rad_ratio, vec2(-1.0, +1.0), +1.0, z1, v2g_c[1]);

    EndPrimitive();
}
