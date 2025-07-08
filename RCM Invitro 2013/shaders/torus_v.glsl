#version 330

uniform mat4 projection, modelview;

in vec4 vertex;
in vec2 coord;

out VertexOut
{
    noperspective vec3 opos;
    noperspective vec3 vpos;
    
} v_out;

void main()
{
    v_out.opos = vertex.xyz;

    vec4 vpos = modelview * vertex;
    
    vec3 res=vec3(128.0);
    vpos.xyz = floor(vpos.xyz*res)/res;
    
    v_out.vpos = vpos.xyz;
    
    gl_Position = projection * vpos;
    
    gl_Position.xy = floor(gl_Position.xy / gl_Position.w * 256.0) / 256.0 * gl_Position.w;
}

