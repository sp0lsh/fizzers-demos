#version 330

#define id gl_VertexID

// vertex shader for drawing particles

uniform sampler2D particle_data_textures[4];
uniform sampler2D particle_triangle_tex;
uniform int particle_data_texture_size;
uniform mat4 modelview;
uniform mat4 projection_inv;
uniform float time;

flat out int v2g_id;
flat out float v2g_morph;

void main()
{
    v2g_id = id;
    
    ivec2 loc = ivec2(id & (particle_data_texture_size - 1), id / particle_data_texture_size);
    
    vec4 dat0 = texelFetch(particle_data_textures[0], loc, 0);
    vec4 dat1 = texelFetch(particle_data_textures[1], loc, 0);
    vec4 dat2 = texelFetch(particle_data_textures[2], loc, 0);
    vec4 dat3 = texelFetch(particle_data_textures[3], loc, 0);
    
    vec4 dat = mix(mix(dat1, dat3, cos(time * 0.2) * 4.0), dat0, (smoothstep(0.0, 0.1, fract(time * 0.1)) - smoothstep(0.1, 0.15, fract(time * 0.1))) * 0.2);
    
    //vec4 dat = mix(dat1, dat3, cos(time * 0.2) * 4.0);

    dat = mix(dat, dat2, 10.0 * smoothstep(3.0, 0.0, time) );
    
    gl_Position = modelview * vec4(dat.xyz, 1);

    gl_Position = mix(gl_Position, projection_inv * vec4(texelFetch(particle_triangle_tex, loc, 0).rgb,1.0),
            pow(smoothstep(30.0, 33.0, time+(0.5+0.5*cos(float(id)))*2.0),2.0) - gl_Position.z * 0.001);
    
    
    //dat = mix(dat, dat0, abs(gl_Position.z - -1.2) * 0.002);
    //gl_Position = modelview * vec4(dat.xyz, 1);

    //v2g_morph = 1.0 -   length(dat.xyz);
    v2g_morph = dat.a;
    
}

