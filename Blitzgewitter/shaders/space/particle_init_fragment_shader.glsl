#version 330

// fragment shader for initialising particle data texture

#define EPS 1e-2
#define PI 3.1415926

uniform sampler2D particle_data_texture;

noperspective in vec2 v2f_coord;

out vec4 f2a_col;

//
// Description : Array and textureless GLSL 2D simplex noise function.
// Author : Ian McEwan, Ashima Arts.
// Maintainer : ijm
// Lastmod : 20110822 (ijm)
// License : Copyright (C) 2011 Ashima Arts. All rights reserved.
// Distributed under the MIT License. See LICENSE file.
// https://github.com/ashima/webgl-noise
//

vec3 mod289(vec3 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec2 mod289(vec2 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec3 permute(vec3 x) {
  return mod289(((x*34.0)+1.0)*x);
}

float snoise(vec2 v)
  {
  const vec4 C = vec4(0.211324865405187, // (3.0-sqrt(3.0))/6.0
                      0.366025403784439, // 0.5*(sqrt(3.0)-1.0)
                     -0.577350269189626, // -1.0 + 2.0 * C.x
                      0.024390243902439); // 1.0 / 41.0
// First corner
  vec2 i = floor(v + dot(v, C.yy) );
  vec2 x0 = v - i + dot(i, C.xx);

// Other corners
  vec2 i1;
  //i1.x = step( x0.y, x0.x ); // x0.x > x0.y ? 1.0 : 0.0
  //i1.y = 1.0 - i1.x;
  i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
  // x0 = x0 - 0.0 + 0.0 * C.xx ;
  // x1 = x0 - i1 + 1.0 * C.xx ;
  // x2 = x0 - 1.0 + 2.0 * C.xx ;
  vec4 x12 = x0.xyxy + C.xxzz;
  x12.xy -= i1;

// Permutations
  i = mod289(i); // Avoid truncation effects in permutation
  vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
+ i.x + vec3(0.0, i1.x, 1.0 ));

  vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
  m = m*m ;
  m = m*m ;

// Gradients: 41 points uniformly over a line, mapped onto a diamond.
// The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)

  vec3 x = 2.0 * fract(p * C.www) - 1.0;
  vec3 h = abs(x) - 0.5;
  vec3 ox = floor(x + 0.5);
  vec3 a0 = x - ox;

// Normalise gradients implicitly by scaling m
// Approximation of: m *= inversesqrt( a0*a0 + h*h );
  m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );

// Compute final noise value at P
  vec3 g;
  g.x = a0.x * x0.x + h.x * x0.y;
  g.yz = a0.yz * x12.xz + h.yz * x12.yw;
  return 130.0 * dot(m, g);
}

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

float nn(vec2 p)
{
    float r = 0.0;
    for(int i = 0; i < 12; i += 1)
    {
        r += (snoise(p * exp2(i)) - 0.5) / exp2(i + 1);
    }
    return r + 0.5;
}

void main()
{
    vec4 rand = texture(particle_data_texture, v2f_coord.xy);

    if(rand.a > 0.02)
    {
        f2a_col.rgb = rotateY(v2f_coord.y * PI * 1241.5492, rotateX(v2f_coord.x * 2.0 * PI, vec3(0.0, 0.0, 1.0)));// * vec3(1.0, 0.0, 1.0) * 2.0;
        f2a_col.a = pow(nn(v2f_coord * 2.0), 1.0);// pow((0.5 + 0.5 * cos(f2a_col.r * 10.0)) * 0.2, 2.0);// * length(f2a_col.rgb);
    }
    else
    {
        //f2a_col.rgb = (rand.xyz - vec3(0.5)) * 2.0;
        f2a_col.rgb = rand.xyz * 2.0;
        f2a_col.a = 0.5 + rand.a * 10.0;
    }
    
    //f2a_col.rgb *= 0.5 + 0.5 * cos(v2f_coord.x * 1030.0) * sin(v2f_coord.y * 1050.0);
}

