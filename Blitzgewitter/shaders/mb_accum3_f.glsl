#version 330

uniform sampler2D tex0;
uniform float time, scale;
uniform float ltime;
uniform float kal;
uniform float brightness;

noperspective in vec2 v2f_coord;

out vec4 output_colour;

vec2 rotate(float a,vec2 v)
{
    return vec2(cos(a)*v.x+sin(a)*v.y,
                cos(a)*v.y-sin(a)*v.x);
}

vec2 tex(vec2 p)
{
   p=abs(fract(p)-vec2(0.5));

   float s=1.0;
   for(int i=0;i<5;++i)
   {
      p=rotate(3.1415926/3.0+ltime*0.1+time*0.01,p);
      p.x=abs(p.x)-0.5;
      p*=2.0;
      s/=2.0;
   }

   return p*0.1;//*vec2(1.0,0.2)*s;
}

vec2 tex2(vec2 p)
{
   p=abs(fract(p)-vec2(0.5));

   float s=1.0;
   for(int i=0;i<8;++i)
   {
      p=rotate(3.1415926/3.0+ltime*0.1+time*0.01,p);
      p.x=abs(p.x)-0.5;
      p*=2.0;
      s/=2.0;
   }

   return p*0.1;//*vec2(1.0,0.2)*s;
}

float Scale=1.3;
vec2 Offset=vec2(0.0,-1.0);
int Iterations=4;

void main()
{
    vec2 coord=(v2f_coord*2.0-vec2(1.0));//*0.75;

    if(kal > 2.0)
    {
       float fa=3.1415926*0.25;
       float sa=3.1415926*0.125;//cos(time);

        vec2 z=coord;
       
        int n = 0;
        while (n < 2) {
           z=rotate(fa,z);      
           if((z.x+z.y)>0.0) z.xy = -z.xy; // fold 1
           if((z.x-z.y)<0.0) z.xy = z.yx; // fold 2
           z=rotate(sa+time*0.01,z);      
           z = z*Scale - Offset*(Scale-1.0);
           n++;
        }

        coord=z;
    }
    else if(kal > 1.5)
    {
        coord=tex(coord);
    }
    else if(kal > 0.5)
    {
       float fa=3.1415926*0.25;
       float sa=3.1415926*0.125;//cos(time);

        vec2 z=coord;
       
        int n = 0;
        while (n < Iterations) {
           z=rotate(fa,z);      
           if((z.x+z.y)>0.0) z.xy = -z.xy; // fold 1
           if((z.x-z.y)<0.0) z.xy = z.yx; // fold 2
           z=rotate(sa+time*0.02,z);      
           z = z*Scale - Offset*(Scale-1.0);
           n++;
        }

        coord=z;
    }
    coord=coord*0.5+vec2(0.5);
    coord=fract(coord);
    output_colour = texture2D(tex0, coord) * scale * brightness;
}

