#version 330

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform vec2 radii;
uniform mat4 projection;
uniform float time;

in int vid[1];

out vec2 bri_tofs;

noperspective out vec2 lcoord;

void emit(vec2 c)
{
    vec2 radii2 = radii * mix(1.0,0.75,smoothstep(0.0,4.0,time));
    float fid = float(vid[0]);
    lcoord = c;
    c*=vec2(0.8+0.2*cos(time*5.0+2.0*fid),0.8+0.2*sin(time*5.0+2.0*fid));
    vec3 ofs = vec3(0.1*time,2.0*time*(0.5+0.5*cos(fid)) - sqrt(time)*6.0 + 5.0,0.0);
    gl_Position = projection * vec4(gl_in[0].gl_Position.xyz + vec3(c * radii2, 0.0) + ofs, 1.0);
    bri_tofs.x = (1.0-smoothstep(0.0,1.0,time))*0.3+mix(0.5,1.0,smoothstep(0.0,2.5,time))*sqrt(0.5+0.5*sin(fid*2.3));
    bri_tofs.y = sin(fid*11.1);
    EmitVertex();
}

void main()
{
    emit(vec2(-1.0, -1.0));
    emit(vec2(+1.0, -1.0));
    emit(vec2(-1.0, +1.0));
    emit(vec2(+1.0, +1.0));
    EndPrimitive();
}
