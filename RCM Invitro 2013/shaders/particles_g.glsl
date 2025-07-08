#version 330

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform float radius;
uniform float aspect_ratio;

in VertexOut
{
    noperspective vec2 tc;
    
} v_out[];

out GeometryOut
{
    noperspective vec2 tc;
    noperspective vec2 coord;
    
} g_out;


void emit(vec2 p, vec2 c)
{
    g_out.coord = c;
    g_out.tc = v_out[0].tc;
    gl_Position.xy = p;
    gl_Position.zw = vec2(0.0, 1.0);
    EmitVertex();
}

void main()
{
    vec2 pos = gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;

    emit(pos + vec2(-aspect_ratio, -1.0) * radius, vec2(-1.0, -1.0));
    emit(pos + vec2(+aspect_ratio, -1.0) * radius, vec2(+1.0, -1.0));
    emit(pos + vec2(-aspect_ratio, +1.0) * radius, vec2(-1.0, +1.0));
    emit(pos + vec2(+aspect_ratio, +1.0) * radius, vec2(+1.0, +1.0));
    
    EndPrimitive();
}
