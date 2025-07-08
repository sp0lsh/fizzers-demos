#version 400

uniform sampler2D tex0;
uniform vec2 direction;
uniform vec4 tint;
uniform vec2 res;
uniform float time;

noperspective in vec2 v2f_coord;

out vec4 output_colour;

void main()
{
    vec4 neighbours=textureGather(tex0,v2f_coord,0);

    vec2 norm=vec2(neighbours.y-neighbours.x, neighbours.w-neighbours.x)*res/vec2(640.0,360.0);
    
    output_colour.a=0.0;

    float a=(smoothstep(0.0,0.2,neighbours.x)-smoothstep(0.2,1.8,neighbours.x))*pow(clamp(norm.y,0.0,1.0),0.9);

    output_colour.rgb=smoothstep(0.11,0.15,a)*vec3(0.6,0.7,1.0)*0.7*0.5;
    
    output_colour.rgb+=(smoothstep(0.0,0.2,neighbours.x)-smoothstep(0.2,1.8,neighbours.x))*clamp(pow(norm.y,1.0),0.0,1.0)*vec3(0.6,0.8,1.0);

    output_colour.rgb+=pow(vec3(smoothstep(0.0,0.2,neighbours.x)-smoothstep(0.2,1.8,neighbours.x))*clamp(-norm.y,0.0,1.0)*0.4*0.5,vec3(1.2));
 
    output_colour.rgb*=2.0*mix(0.95,1.0,0.5+0.5*cos(time*20.0)) *
                        (1.0 + clamp((smoothstep(0.1,0.12,neighbours.x)-smoothstep(0.12,0.2,neighbours.x)),0.0,1.0)*6.0)*vec3(0.6,0.8,1.0);

}
