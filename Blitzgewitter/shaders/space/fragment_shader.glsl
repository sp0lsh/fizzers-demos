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
    float a = 1e10;
    float b = 0.0;
    
    p = rotateX(time, p);
    
    for(int z = -1; z <= +1; z += 1)
        for(int y = -1; y <= +1; y += 1)
            for(int x = -1; x <= +1; x += 1)
            {
                a = min(a, distance(p, rotateX(time + b, rotateY(time + b, vec3(ivec3(x, y, z))) * 0.8)) - 0.5);
                b += 1.0;
            }

    return -abs(a * a) * 1e-2;
            
            
/*
    // cool 1
    float a = 1e10;
    float b = 0.0;
    
    p = rotateX(time, p);
    
    for(int z = -1; z <= +1; z += 1)
        for(int y = -1; y <= +1; y += 1)
            for(int x = -1; x <= +1; x += 1)
            {
                a = min(a, distance(p, rotateX(time + b, rotateY(time + b, vec3(ivec3(x, y, z))) * 0.8)) - 0.6);
                b += 1.0;
            }

    return -abs(a * a) * 1e-2;
*/


/*
    // cool 2
    float a = 1e10;
    float b = 0.0;
    
    p = rotateX(time, p);
    
    for(int z = -1; z <= +1; z += 1)
        for(int y = -1; y <= +1; y += 1)
            for(int x = -1; x <= +1; x += 1)
            {
                a = min(a, pow(distance(p, rotateX(time + b, rotateY(time + b, vec3(ivec3(x, y, z))) * 0.4)) - 0.7 - time * 0.1, 3.0));
                b += 1.0;
            }

    return -abs(a * a) * 1e-2;
*/

/*
    // cool 3
    float a = 1e10;
    float b = 0.0;
    
    p = rotateX(time, p);
    
    for(int z = -1; z <= +1; z += 1)
        for(int y = -1; y <= +1; y += 1)
            for(int x = -1; x <= +1; x += 1)
            {
                a = min(a, distance(p, rotateX(time + b, rotateY(time + b, vec3(ivec3(x, y, z))) * 0.4)) - 0.7 - time * 0.1 + cos(p.x * 10.0) * 0.1);
                b += 1.0;
            }

    return -abs(a * a) * 1e-2;
*/
            
            
            /*
    float d = pow(min(distance(p, vec3(0.5, 0.5, 0.5)) - 0.4,
                        distance(p, vec3(1.0, 0.1, 0.1)) - 0.3), 2.0);

    return -abs(d) * 1e-2;


    p = rotateX(p.x * 2.0, p);

    float a = 0.0;
    
    for(int z = -1; z <= +1; z += 1)
        for(int y = -1; y <= +1; y += 1)
            for(int x = -1; x <= +1; x += 1)
            {
                a += 1.0 / (distance(p, vec3(ivec3(x, y, z)) * 1.2) - (0.2 + 0.1 * cos(p.x * 10.0) * cos(p.y * 3.0) * cos(p.z * 7.0)));
            }

    return a * 0.03;
            
    vec3 ip = floor(p), fp = fract(p);

    return -abs((distance(fp, vec3(0.5)) - 0.4));
    
    return distance(p, vec3(0.2, 0.1, 0.1)) * 2.0 -
           distance(p, vec3(-0.2, 0.1, 0.1)) * 4.0 +
           distance(p, vec3(-0.5, -0.5, -1.0)) * 3.0 +
           distance(p, vec3(0.5, 0.5, 0.1)) * 3.0 +
           distance(p, vec3(-0.5, -0.5, 1.0)) * 5.0;
           */
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
    
     //dat.xyz += vectorField(dat.xyz) * smoothstep(6.0, 8.0, time) * 3.0 + normalize(dat.xyz - vec3(0.0)) * smoothstep(8.0, 10.0, time);

     // TRY THIS: generate the particle starting points on a mesh
     
     //f2a_col.xyz = vectorField(dat.xyz) * mix(0.2, 4.0, 1.0 - dat.w);
     f2a_col.xyz = vectorField(dat.xyz) * mix(0.2, 4.0, dat.w);
     
     /*
   int depth = 7;
   vec3 box_min = vec3(-1.0, -1.0, -1.0);
   vec3 box_max = vec3(+1.0, +1.0, +1.0);
   vec3 pnt = dat.xyz;
   int index = 0;
   
   for(int i = 0; i < depth; i += 1)
   {
      //int axis = (i + int(floor(time))) % 3;
      int axis = i % 3;
      float mid = mix(box_min[axis], box_max[axis], sin(float(index) * 115.5) * 0.1 + 0.5);
      if(pnt[axis] < mid)
      {
         box_max[axis] = mid;
         index = (index << 1) | 0;
      }
      else
      {
         box_min[axis] = mid;
         index = (index << 1) | 1;
      }
   }
    
   vec3 box_center = (box_min + box_max) * 0.5;
   //vec3 local = (pnt - box_min) / (box_max - box_min);
   
   vec3 diff = abs(pnt - box_center);
   
    if(all(greaterThan(diff.xx, diff.yz)))
    {
        dat.x += ( mix(box_min.x, box_max.x, step(box_center.x, dat.x))  - dat.x) * 0.02;
    }
    else if(all(greaterThan(diff.yy, diff.xz)))
    {
        dat.y += ( mix(box_min.y, box_max.y, step(box_center.y, dat.y))  - dat.y) * 0.02;
    }
    else
    {
        dat.z += ( mix(box_min.z, box_max.z, step(box_center.z, dat.z))  - dat.z) * 0.02;
    }
   */
   
    //f2a_col = dat;
}

