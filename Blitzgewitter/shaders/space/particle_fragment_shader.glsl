#version 330

// fragment shader for drawing particles

uniform float time;
uniform float cscale;
noperspective in vec2 g2f_coord;
flat in int g2f_id;
flat in float g2f_morph;

out vec4 f2a_col;


void main()
{
    int type = g2f_id & 1023;

    float mask = clamp(1.0 - dot(g2f_coord.xy, g2f_coord.xy), 0.0, 1.0);
    
    mask = mix(mask / (1.01 - mask), mask * 3.0, (1.0 - g2f_morph));
    
    f2a_col.a = 1.0;
    
    f2a_col.rgb = ((smoothstep(0.0, 0.1, g2f_morph) - smoothstep(0.5, 0.6, g2f_morph)) * mask * vec3(0.4, 0.32, 0.4) * 0.2 +
                  smoothstep(0.2, 0.6, g2f_morph) * 0.2 * mask * vec3(1.0, 1.0, 0.4)) * 0.2;
    
    f2a_col.rgb += smoothstep(0.1, 0.0, g2f_morph) * clamp(1.0 - dot(g2f_coord.xy, g2f_coord.xy), 0.0, 1.0) * vec3(0.0, 0.25, 0.5) * 0.01;
    
    f2a_col.rgb *= 0.15;
    
    f2a_col.rgb += vec3(0.002) * smoothstep(33.0, 35.0, time);
    f2a_col.rgb *= cscale;
}
