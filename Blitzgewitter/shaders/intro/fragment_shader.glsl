#version 330

// fragment shader for updating particle data texture

#define EPS 1e-2

uniform sampler2D particle_data_texture;
uniform float time;

noperspective in vec2 v2f_coord;
out vec4 f2a_col;

vec3 rotateX(float a, vec3 v)
{
   return vec3(v.x,
               cos(a) * v.y + sin(a) * v.z,
               cos(a) * v.z - sin(a) * v.y);
}

vec3 rotateY(float a, vec3 v)
{
   return vec3(cos(a) * v.x + sin(a) * v.z,
               v.y,
               cos(a) * v.z - sin(a) * v.x);
}

float f(vec3 p)
{
    float a = 1e6;
    float b = 0.0;

    float time2=sqrt(time*10.0);
    
    p = rotateX(time2, p);
    
    for(int z = -1; z <= +1; z += 1)
        for(int y = -1; y <= +1; y += 1)
            for(int x = -1; x <= +1; x += 1)
            {
                a = min(a, distance(p, rotateX(time2 + b, rotateY(time2 + b, vec3(ivec3(x, y, z))) * 0.8)) - 0.5);
                b += 1.0;
            }

    return -abs(a * a) * 1e-2;
}

vec3 vectorField(vec3 p)
{
    float ff = f(p);
    
    vec3 df = vec3( f(p + vec3(EPS, 0.0, 0.0)) - ff,
                    f(p + vec3(0.0, EPS, 0.0)) - ff,
                    f(p + vec3(0.0, 0.0, EPS)) - ff) / EPS;

    return df;
}

void main()
{
    vec4 dat = texture(particle_data_texture, v2f_coord);
    f2a_col.xyz = dat.xyz + vectorField(dat.xyz) * mix(0.2, 4.0, dat.w);
}

