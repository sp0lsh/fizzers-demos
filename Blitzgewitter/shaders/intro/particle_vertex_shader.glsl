#version 330

// vertex shader for drawing particles

uniform sampler2D particle_data_textures[4];
uniform int particle_data_texture_size;
uniform mat4 modelview;
uniform float time;
uniform float ftime;
uniform int id_offset;

layout(location=4) in vec2 pcoord;

flat out uint v2g_id;
flat out float v2g_morph;

void main()
{
    uint id = uint(gl_VertexID);

    v2g_id = id;

    vec4 dat = mix(textureLod(particle_data_textures[0], pcoord, 0), textureLod(particle_data_textures[1], pcoord, 0), ftime);
    
    float l=sqrt(length(dat.xyz));
    dat.xyz=normalize(dat.xyz)*l;
    
    gl_Position = modelview * vec4(dat.xyz, 1.0);

    v2g_morph = dat.a+cos(id)*0.5;
}
