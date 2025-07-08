#version 330


layout(lines) in;
layout(triangle_strip, max_vertices = 8) out;

in float v2g_cd[2];

uniform vec2 radii;

noperspective out vec3 lcoord;
noperspective out float zcoord;

void emit(vec2 p, vec2 c, float t, float z, float cd)
{
    lcoord.xy = c;
    lcoord.z = t;
    zcoord = z;
    gl_Position.xy = p;
    gl_Position.zw = vec2(0.0, 1.0);
    gl_ClipDistance[0] = cd;
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

    emit(pos0 + prp * (+1.0) + pll * (-1.0), vec2(+1.0, -1.0), -1.0, z0, v2g_cd[0]);
    emit(pos0 + prp * (-1.0) + pll * (-1.0), vec2(-1.0, -1.0), -1.0, z0, v2g_cd[0]);

    emit(pos0 + prp * (+1.0), vec2(+1.0, 0.0), -1.0, z0, v2g_cd[0]);
    emit(pos0 + prp * (-1.0), vec2(-1.0, 0.0), -1.0, z0, v2g_cd[0]);

    emit(pos1 + prp * (+1.0) * rad_ratio, vec2(+1.0, 0.0), +1.0, z1, v2g_cd[1]);
    emit(pos1 + prp * (-1.0) * rad_ratio, vec2(-1.0, 0.0), +1.0, z1, v2g_cd[1]);
 
    emit(pos1 + prp * (+1.0) * rad_ratio + pll * (+1.0) * rad_ratio, vec2(+1.0, +1.0), +1.0, z1, v2g_cd[1]);
    emit(pos1 + prp * (-1.0) * rad_ratio + pll * (+1.0) * rad_ratio, vec2(-1.0, +1.0), +1.0, z1, v2g_cd[1]);

    EndPrimitive();
}
