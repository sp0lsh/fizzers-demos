#version 330

uniform sampler2D tex0, noise_tex;
uniform float time, scale, subtime;

noperspective in vec2 v2f_coord;

out vec4 output_colour;

void main()
{
    vec2 ofs = vec2(pow(texture2D(noise_tex, vec2(time*0.3,time*0.2+v2f_coord.y*0.1)).r,4.0),
                    pow(texture2D(noise_tex, vec2(time*0.1,time*0.1+v2f_coord.x*0.1)).r,2.0))*0.2/pow(1.0+time*0.4,2.0);
                    
    output_colour = texture2D(tex0, v2f_coord+ofs) * scale;
}

