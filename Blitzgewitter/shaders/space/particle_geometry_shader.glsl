#version 330

// geometry shader for drawing particles

uniform mat4 projection;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

flat in int v2g_id[1];
flat in float v2g_morph[1];

noperspective out vec2 g2f_coord;
flat out int g2f_id;
flat out float g2f_morph;

void main()
{
    float size = 0.004;
 
    /*
    int type = v2g_id[0] & 1023;

    switch(type)
    {
        case 0:
            size *= 40.0;
            break;
    }
 */
 
    size = mix(0.006, 0.002, v2g_morph[0]) * 1.0 + smoothstep(0.1, 0.0, v2g_morph[0]) * 0.005;
 
    //size *= 3.0;
 
    gl_Position = projection * (gl_in[0].gl_Position + vec4(-size, -size, 0.0, 0.0));
    g2f_coord = vec2(-1.0, -1.0);
    g2f_id = v2g_id[0];
    g2f_morph = v2g_morph[0];
    EmitVertex();

    gl_Position = projection * (gl_in[0].gl_Position + vec4(+size, -size, 0.0, 0.0));
    g2f_coord = vec2(+1.0, -1.0);
    g2f_id = v2g_id[0];
    g2f_morph = v2g_morph[0];
    EmitVertex();

    gl_Position = projection * (gl_in[0].gl_Position + vec4(-size, +size, 0.0, 0.0));
    g2f_coord = vec2(-1.0, +1.0);
    g2f_id = v2g_id[0];
    g2f_morph = v2g_morph[0];
    EmitVertex();

    gl_Position = projection * (gl_in[0].gl_Position + vec4(+size, +size, 0.0, 0.0));
    g2f_coord = vec2(+1.0, +1.0);
    g2f_id = v2g_id[0];
    g2f_morph = v2g_morph[0];
    EmitVertex();
    
    EndPrimitive();
}
