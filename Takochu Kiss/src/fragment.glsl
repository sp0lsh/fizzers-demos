#version 130

// union
vec2 u(vec2 a, vec2 b)
{
   return mix(a, b, step(b.y, a.y));
}

float box(vec3 p, vec3 s)
{
   return max(max(abs(p.x) - s.x, abs(p.y) - s.y), abs(p.z) - s.z);
}

float smin(float a,float b,float k){ return -log(exp(-k*a)+exp(-k*b))/k;}//from iq

mat2 rotmat(float a)
{
   return mat2(cos(a),sin(a),-sin(a),cos(a));
}

float takochu(vec3 p)
{
   vec3 mp=vec3(0,-.08,.52);
   float m=distance(p,vec3(mp.xy+normalize(p.xy-mp.xy)*.08,clamp(p.z,0,mp.z)))-.052;
   float b=length(p)-.5;
   vec3 tp=vec3(0,-.4,.4)*.97;
   vec3 p2=p;
   p2.xz=abs(p2.xz);
   if(p2.z>p2.x)
      p2.xz=p2.zx;
   p2.xz*=rotmat(-acos(-1.)/2.65);
   p2.yz=(p2.yz-tp.yz)*rotmat(-.7)+tp.yz;
   float t=distance(p2,vec3(tp.xy+normalize(p2.xy-tp.xy)*clamp(length(p2.xy-tp.xy),0.,.1),tp.z))-.04;
   float bbr=length(p)-1.;
   if(bbr>1)
      return bbr;
   return smin(smin(b,m,50),t,30);
}

float eyes(vec3 p)
{
   p.x=abs(p.x);
   return step(0,distance(p,vec3(.35,-.06,.5))-.122);
}

float mouth(vec3 p)
{
   vec3 mp=vec3(0,-.08,.52);
   float m=distance(p,vec3(mp.xy+normalize(p.xy-mp.xy)*.08,clamp(p.z,0,mp.z)))-.022;
   return step(.005,m);
}

vec2 takochu2(vec3 p,float takm)
{
   if(eyes(p)<.5)
      takm=4;
   if(mouth(p)<.5)
      takm=5;
   return vec2(takm, takochu(p));
}


float nofloor=0;
float material=0;
vec2 f2(vec3 p)
{
   vec3 op=p;

   vec2 bd=vec2(0,100);

if(nofloor<.5)
bd = vec2(0,p.y+.48);// min(-box(p, vec3(20,60,20)),p.y+9.);

   bd=u(bd,takochu2(op,1));
   op+=vec3(1.1,0,0);
   op.xz*=rotmat(-1.4);
   bd=u(bd,takochu2(op,1));

material=bd.x;
   return bd;
}


float f(vec3 p2)
{
   return f2(p2/6).y*6-.14;
}


float box(vec3 p,vec3 s,vec3 o)
{
   p=abs(p-o);
   return max(p.x-s.x,max(p.y-s.y,p.z-s.z));
}

float roundbox(vec3 p,vec3 s,vec3 o)
{
   return length(max(vec3(0),abs(p-o)-s));
}

float dist(vec3 p)
{
    return min(min(min(min(f(p/8.)*8.,roundbox(p,vec3(10,10,0),vec3(0,50,0))-4),
         roundbox(p,vec3(10,10,0),vec3(-53,50,0))-4),
         roundbox(p,vec3(0,0,0),vec3(-53,32,0))-3),
         roundbox(p,vec3(0,0,0),vec3(0,32,0))-3);
}


float line(vec3 p,vec3 l0,vec3 l1)
{
  return distance(p,mix(l0,l1, clamp(dot(p-l0,normalize(l1-l0))/distance(l0,l1),0.,1.)));
}

vec4 image(vec2 t)
{
nofloor=0;
material=0;

vec4  color=vec4(0);
vec3 fog=vec3(.5,.5,1);
float mat=0;

   vec2 p=t.xy;

   vec3 ru=normalize(vec3(1,0,0));
   vec3 rv=normalize(vec3(0,1,0));
   vec3 rw=normalize(cross(rv,ru));
   float aa=.4;
   ru=cos(aa)*ru+sin(aa)*rw;
   rw=normalize(cross(rv,ru));
   aa=0.5;
   rv=cos(aa)*rv+sin(aa)*rw;
   rw=normalize(cross(rv,ru));

   vec3 ro=vec3(-11,35,40)+(p.x*ru+p.y*rv)*100;
   vec3 rd=rw+1e-3;

   vec3 co=ro;
   rd=normalize(rd);
   vec3 ord=rd;

   vec3 f=vec3(1.),a=vec3(0.);
vec3 n;
for(int j=0;j<4;++j)
{
   vec3 oro=ro;

   float t=-7;
   for(int i=0;i<30;++i)
   {
      ro=oro+rd*t;
      float d=dist(ro);
      if(d<1e-4)
      {
         break;
      }
      t+=d;
   }


   oro=ro;
   vec3 c=floor(ro);
   vec3 ts=(c+max(vec3(0.),sign(rd))-ro)/rd;

   for(int i=0;i<30;++i)
   {
      n=step(ts,ts.yzx)*step(ts,ts.zxy);
      c+=sign(rd)*n;
      float d=dist(c);
      vec3 dd=sign(rd)/rd*n;
      if(d<-1.4)
      {
         ro=oro+rd*min(ts.x,min(ts.y,ts.z));
         break;
      }
      ts+=dd;
   }
         ro=oro+rd*min(ts.x,min(ts.y,ts.z));
}

float fogstrength=.0013;


float sh=1;
{
   ro-=rd*1e-5;
   vec3 oro=ro;
   vec3 rd=normalize(vec3(1,2,-.5));
   vec3 c=floor(ro);
   vec3 ts=(c+max(vec3(0.),sign(rd))-ro)/rd,n;

   for(int i=0;i<60;++i)
   {
      n=step(ts,ts.yzx)*step(ts,ts.zxy);
      c+=sign(rd)*n;
      float d=dist(c);
      vec3 dd=sign(rd)/rd*n;
      if(d<-1.4)
      {
         sh=0.4;
         //ro=oro+rd*min(ts.x,min(ts.y,ts.z));
         break;
      }
      ts+=dd;
   }
}


vec3 uv=ro*(vec3(1)-n);

vec3 diff=vec3(1);
diff*=mix(.25,1,smoothstep(0.0,40.0,length((vec3(0,-18,0)*1.5-ro)*vec3(1,3,1))));
diff*=mix(.25,1,smoothstep(0.0,40.0,length((vec3(-35,-18,0)*1.5-ro)*vec3(1,3,1))));

dist(ceil(ro));
mat=material;

if(ro.y<-22.9)
{
nofloor=1;
diff*=mix(.8,1.,smoothstep(-4,2,dist(ro)));
diff*=vec3(1,.7,.45);
}
else
{
diff*=mix(vec3(1),vec3(1,.7,.45),.5*(1.-smoothstep(0,55,ro.y+22)));
if(ro.x<-24&&ro.y<30)
   diff*=vec3(1,.9,1);

if(box(floor(ro-.001),vec3(1,5,10),vec3(10,4,20))<0)
   diff*=.35;

if(box(floor(ro-.001),vec3(1,5,10),vec3(-10,4,20))<0)
   diff*=.35;


if(box(floor(ro-.001),vec3(10,1,4),vec3(-35,4,15))<0)
   diff*=.35;


if(length(floor(ro).xy-vec2(0,-4))<2)
   diff*=.35;

if(length(floor(ro).xy-vec2(12,-6))<3.5)
   diff*=vec3(1,.8,.8);

if(length(floor(ro).xy-vec2(-12,-6))<3.5)
   diff*=vec3(1,.8,.8);

   float ausrufzeichen=min(roundbox(floor(ro),vec3(0,4,0),vec3(0,53,4))-2.5,
roundbox(floor(ro),vec3(0,0,0),vec3(0,42,4))-2.5);
if(ausrufzeichen<0)
   diff*=.55;

float herz=min(line(floor(ro),vec3(-53,47,0),vec3(-53+5,47+5,0)),
line(floor(ro),vec3(-53,47,0),vec3(-53-5,47+5,0)))-5.95;
if(herz<-.9)
   diff*=vec3(1,.8,.8);
else if(herz<0)
   diff*=.55;

if(roundbox(floor(ro),vec3(0),vec3(-53+5,47+6,0))<3.2)
   diff=vec3(1);

}

color.a=1;

   color.rgb=diff*(.75+.25*dot(n*-sign(rd),normalize(vec3(1,2,1.4))))*
         smoothstep(-3.,-1,dist(ro-vec3(.1)))*mix(vec3(.6,.6,.8),vec3(1),sh);

color*=mix(.7,1.,smoothstep(0.,40.,distance(ro,vec3(-65,-10,2))));
color*=mix(.7,1.,smoothstep(0.,40.,distance(ro,vec3(-8,-10,2))));

   float fogamount=exp(-distance(ro,co)*fogstrength);
   color.rgb=mix(fog,color.rgb,fogamount);

color*=max(min(1.,exp(-(length(t.xy)-.5)*1))/3, step(length(t.xy),.7))*1.12;
//color*=step(length(t.xy),.7)*1.12;
return color;
}

void main()
{
vec2 t=gl_FragCoord.xy/vec2(1920,1080)*2.-vec2(1);
t.x*=1920./1080.;
t*=.8;

gl_FragColor=vec4(0);
for(int y=-1;y<=1;++y)
for(int x=-1;x<=1;++x)
   gl_FragColor+=image(t.xy+vec2(x,y)*.0005);
gl_FragColor/=9.;
gl_FragColor.rgb+=vec3(cos(gl_FragCoord.y*7)*cos(gl_FragCoord.x*5)*.5+.5)/100.;
}

