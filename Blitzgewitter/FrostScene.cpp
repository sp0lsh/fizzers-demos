
#include "Engine.hpp"
#include "Bezier.hpp"
#include "Ran.hpp"
#include "Particles.hpp"
#include "Tower.hpp"

#include <list>

extern tower::Tower* platonic2_tower;

static inline Real frand()
{
   static Ran rnd(643);
   return rnd.doub();
}

static long int lrand()
{
   static Ran rnd(112);
   return rnd.int32();
}

static Vec3 normalize(const Vec3& v)
{
   Real rl=Real(1)/std::sqrt(v.lengthSquared());
   return v*rl;
}

static const float open_time=5.7f;

static double g_ltime=0;

static const float net_ball_radius = 0.1f;
static const float net_cell_width = 2.0f;
static const float net_cell_height = 1.0f;
static const float net_cell_x_shift = 1.0f;
static const float net_core_radius = 0.02f;
static const float net_coil_outer_radius = 0.05f;
static const float net_coil_inner_radius = 0.01f;
static const int net_core_coil_count = 5;
static const float square_root_of_two = 1.4142135623730951f;
static const float net_core_length = square_root_of_two - net_ball_radius * 2.0f;
static const int net_width = 5, net_height = 5;
static const int net_ball_count = net_width * net_height;
static const int net_ball_grid_size = 8;
static const int net_core_divisions = 8;
static const int net_coil_outer_divisions = 8;
static const int net_coil_inner_divisions = 8;
static const int net_pylon_count = (net_height - 1) * net_width + (net_width - 1) * (net_height - 1);

static const int net_ball_vertex_count = 8 + (net_ball_grid_size - 2) * (net_ball_grid_size - 2) * 6 + (net_ball_grid_size - 2) * 12;
static const int net_core_vertex_count = net_core_divisions * 2;
static const int net_coil_vertex_count = net_core_coil_count * net_coil_outer_divisions * net_coil_inner_divisions;
static const int net_pylon_vertex_count = net_core_vertex_count + net_coil_vertex_count;
static const int net_max_vertices = net_ball_count * net_ball_vertex_count + net_pylon_count * net_pylon_vertex_count;

static const int net_ball_index_count = (net_ball_grid_size - 1) * 4 * (net_ball_grid_size - 1) * 2 + (net_ball_grid_size - 1) * net_ball_grid_size * 2 * 2; // TRIANGLE_STRIPs
static const int net_core_index_count = net_core_divisions * 2; // TRIANGLE_STRIPs
static const int net_coil_index_count = net_core_coil_count * net_coil_outer_divisions * 2 * net_coil_inner_divisions; // TRIANGLE_STRIPs
static const int net_pylon_index_count = net_core_index_count + net_coil_index_count; // TRIANGLE_STRIPs
static const int net_max_indices = net_ball_count * net_ball_index_count + net_pylon_count * net_pylon_index_count;

static const int net_ball_strip_count = (net_ball_grid_size - 1) * 3;
static const int net_core_strip_count = 1;
static const int net_coil_strip_count = net_core_coil_count * net_coil_inner_divisions;
static const int net_pylon_strip_count = net_core_strip_count + net_coil_strip_count;
static const int net_max_strips = net_ball_count * net_ball_strip_count + net_pylon_count * net_pylon_strip_count;

namespace lightning
{

float base_branch_length = 160;
float large_noise_warp = 50;
float small_noise_warp = 40;
float grow_shrink_rate = 0.15f;
float grow_shrink_exp = 1.1f;
float max_depth = 5.0f;
float angle_distribution = 0.7f;
float branch_shorten_factor = 0.8f;
float roll_rate_factor = 1.0f;

float noise2(float x,float y=0,float z=0)
{
   static bool init=true;
   static const int size=64;
   static float values[size*size*size];
   if(init)
   {
      Ran lrnd(4343);
      for(int z=0;z<size;++z)
         for(int y=0;y<size;++y)
            for(int x=0;x<size;++x)
            {
               values[x+(y+z*size)*size]=lrnd.doub();
            }
      init=false;
   }

   const int ix=int(x) & (size-1);
   const int iy=int(y) & (size-1);
   const int iz=int(z) & (size-1);

   const float fx=x-floor(x);
   const float fy=y-floor(y);
   const float fz=z-floor(z);

   float vs[8];
   for(int w=0;w<2;++w)
      for(int v=0;v<2;++v)
         for(int u=0;u<2;++u)
         {
            vs[u+(v+w*2)*2]=values[((ix+u)&(size-1))+(((iy+v)&(size-1))+((iz+w)&(size-1))*size)*size];
         }

   float a=(vs[0]*(1.0f-fx)+vs[1]*fx)*(1.0f-fy)+
           (vs[2]*(1.0f-fx)+vs[3]*fx)*fy;

   float b=(vs[4]*(1.0f-fx)+vs[5]*fx)*(1.0f-fy)+
           (vs[6]*(1.0f-fx)+vs[7]*fx)*fy;

   return a*(1.0f-fz)+b*fz;
}

float noise(float x,float y=0,float z=0)
{
   float f=0;
   for(int i=0;i<3;++i)
   {
      float s=float(1<<i);
      f+=noise2(x*s,y*s,z*s)*0.5f/s;
   }

   return f;
}

int fc=0;
int id_counter = 0;

struct Node
{
  Node(Node* parent, float angle)
  {
    id = id_counter++;
    pos = Vec2(0,0);
    this->angle = angle;
    this->parent = parent;
  }

  void grow(int depth, int grow_by)
  {
    if (depth > 0)
    {
      for(std::vector<Node>::iterator it=subnodes.begin();it!=subnodes.end();++it)
        it->grow(depth - 1, grow_by);
    }
    else
      createSubnodes(grow_by);
  }

  void createSubnodes(int depth)
  {
    if (depth <= 0)
      return;

    subnodes.clear();

    for (int i = 0; i < 2; ++i)
    {
      Node sn(this, angle);
      sn.angle += angle_distribution * (i * 2 - 1) + (noise(sn.id) - 0.5) * 2;
      sn.l = l * branch_shorten_factor;
      sn.createSubnodes(depth - 1);
      subnodes.push_back(sn);
    }
  }

  void render(int depth, Vec2 org, float depthf, int depthc, GLushort& num_vertices, GLfloat* vertices, GLfloat* colours)
  {
    if (depth < 0)
      return;

    float tween = 1.0f - (float(depthc) - depthf);

    Vec2 dir=Vec2(cos(angle)*l, sin(angle)*l);
    Vec2 end=org + dir;

    float ti = fc * 0.03 * roll_rate_factor;
    float sx = 0.04, sy = 0.02;
    float ss = 0.3f;
    static const int n = 64;
    //Vec2* prev = 0;

    num_vertices=0;
if(parent)
{
    for (int i = 0; i <= n; ++i)
    {
      float t = std::min(float(i) / float(n), tween);

      //Vec3 col= lerpColor(color(50, 50, 255), color(200, 200, 255), 1.0 / (t + float(1 + depthc))));
      Vec3 col= mix(Vec3(50, 50, 255), Vec3(200, 200, 255), 1.0 / (t + float(1 + depthc)));

      Vec2 mid = ((org * (1.0f - t)) + (end * t));
      Vec2 disp = Vec2(noise(mid.x * sx, mid.y * sy, ti), noise(mid.x * sx, mid.y * sy, ti * 2)) * small_noise_warp;
      mid = mid + disp;
      disp = Vec2(noise(mid.x * sx * ss, mid.y * sy * ss, ti * 1.3),
      noise(mid.x * sx * ss, mid.y * sy * ss, 10 + ti * 1.2)) * large_noise_warp;

      mid = mid + disp;

      colours[num_vertices*3+0]=col.x/255.0f;
      colours[num_vertices*3+1]=col.y/255.0f;
      colours[num_vertices*3+2]=col.z/255.0f;

      vertices[num_vertices*2+0]=mid.x*0.005;
      vertices[num_vertices*2+1]=mid.y*0.005;
      ++num_vertices;

      //if (prev != 0)
      //  line(prev.x, prev.y, mid.x, mid.y);
      //prev = mid;
    }

     glDrawArrays(GL_LINE_STRIP, 0, num_vertices);
}
    //line(org.x, org.y, end.x, end.y);

    //for (Node sn : subnodes)
    for(std::vector<Node>::iterator it=subnodes.begin();it!=subnodes.end();++it)
      it->render(depth - 1, end, depthf, depthc + 1, num_vertices, vertices, colours);
  }

  float l;
  int id;
  Node* parent;
  float angle;
  Vec2 pos;
  std::vector<Node> subnodes;
};

struct Tree
{
  Tree(const Vec2& origin): root(0, 0)
  {
    this->origin = origin;
    root.l = base_branch_length;
  }

  void shrinkOrGrow(float to_depthf)
  {
    int to_depth = ceil(to_depthf);
    depthf = to_depthf;
    if (to_depth < depth)
      depth = to_depth;
    else if (to_depth > depth)
    {
      root.grow(depth, to_depth - depth);
      depth = to_depth;
    }
  }

  void render(GLushort& num_vertices, GLfloat* vertices, GLfloat* colours)
  {
    root.render(depth, origin, depthf, 0, num_vertices, vertices, colours);
  }

  Vec2 origin;
  Node root;
  float depthf;
  int depth;
};



}

class FrostScene: public Scene
{
   struct Worm
   {
      std::list<Vec3> position_history;
      Vec3 position;
      Vec3 direction;
      Real step_count;

      Worm()
      {
         step_count=0;
         position=Vec3(0,0,0);
         direction=Vec3(0,0,0);
      }
   };

   struct Box
   {
      Vec3 size;
      Mat4 xfrm, xfrm_inv;
   };

   static const int num_boxes = 8;
   static const int num_worms = 10;

   Box boxes[num_boxes];

   Worm worms[num_worms];

   static const int random_grid_w = 256;
   static const int random_grid_h = 256;

   float random_grid[random_grid_w*random_grid_h];

   void initRandomGrid();
   float randomGridLookup(float u, float v) const;

   GLfloat spiral_vertices[4096 * 3];
   GLushort spiral_indices[4096];
   GLushort spiral_num_vertices, spiral_num_indices;

   static const GLushort lightning_max_vertices=2048;

   GLfloat lightning_vertices[lightning_max_vertices*2];
   GLfloat lightning_colours[lightning_max_vertices*3];

   Shader forrest_shader, bokeh_shader, bokeh2_shader;
   Shader gaussbokeh_shader, composite_shader;
   Shader mb_accum_shader;
   Shader screendisplay_shader;
   Shader bgfg_shader;
   Shader lightning_shader;
   Shader forrest2_shader;

   Mesh scene_mesh;
   Mesh icosahedron_mesh;

   bool drawing_view_for_background;
   int num_subframes;
   float subframe_scale;

   float lltime;

   GLuint net_strip_offsets[net_max_strips];
   GLuint net_indices[net_max_indices];
   GLfloat net_vertices[net_max_vertices*3];

   void initNet();

   static const int num_fbos = 16, num_texs = 16;

   GLuint fbos[num_fbos], texs[num_texs], renderbuffers[num_fbos];

   GLuint bokeh_temp_fbo;
   GLuint bokeh_temp_tex_1, bokeh_temp_tex_2;

   GLuint noise_tex_2d;

   lightning::Tree litree0,litree1;

   void initializeTextures();
   void initializeShaders();
   void initializeBuffers();

   void initWorms();
   void initBoxes();
   void drawView(float ltime);
   void createSpiral();
   void gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0);
   void drawLightning(float ltime);

   void drawHangingBalls(float ltime);
   void drawTower(tower::Tower& twr, const Mat4& modelview, Real camheight);

   void step();

   void addHistory(Worm& w);
   double sceneDistance(const Vec3& p);
   Vec3 sceneGradient(const Vec3& p);

   void stepWorm(Worm& w,int i);

   public:
      FrostScene(): scene_mesh(256, 256), litree0(Vec2(-200, 0)), litree1(Vec2(-200, 0)), icosahedron_mesh(32, 32)
      {
      }

      ~FrostScene()
      {
         free();
      }

      void slowInitialize();
      void initialize();
      void render();
      void update();
      void free();
};

Scene* frostScene = new FrostScene();

void FrostScene::initNet()
{
   // cores
   static const float core_t0=net_ball_radius;
   static const float core_t1=net_core_length-net_ball_radius;
   for(int i=0;i<net_pylon_count;++i)
   {
      const int x=i%(net_width*2-1);
      const int y=i/(net_width*2-1);
      for(int j=0;j<net_core_divisions;++j)
      {
         const float angle=float(j)/float(net_core_divisions)*2*M_PI;
         const float s=std::sin(angle);
         const float c=std::cos(angle);
         {
            int k = i * net_core_vertex_count + j * 2 + 0;
            net_vertices[k * 3 + 0] = (x / 2) * net_cell_width + (y & 1) * net_cell_x_shift + square_root_of_two * s * net_core_radius + ((x & 1) ? -square_root_of_two : +square_root_of_two) * core_t0;
            net_vertices[k * 3 + 1] = (y / 2) * net_cell_height - ((x & 1) ? +square_root_of_two : -square_root_of_two) * s * net_core_radius + square_root_of_two * core_t0;
            net_vertices[k * 3 + 2] = c * net_core_radius;
         }
         {
            int k = i * net_core_vertex_count + j * 2 + 1;
            net_vertices[k * 3 + 0] = (x / 2) * net_cell_width + (y & 1) * net_cell_x_shift + square_root_of_two * s * net_core_radius + ((x & 1) ? -square_root_of_two : +square_root_of_two) * core_t1;
            net_vertices[k * 3 + 1] = (y / 2) * net_cell_height - ((x & 1) ? +square_root_of_two : -square_root_of_two) * s * net_core_radius + square_root_of_two * core_t1;
            net_vertices[k * 3 + 2] = c * net_core_radius;
         }
      }
   }
}

void FrostScene::step()
{
   for(int i=0;i<num_worms;++i)
      stepWorm(worms[i],i);
}

void FrostScene::addHistory(Worm& w)
{
   w.position_history.push_back(w.position);

   if(w.position_history.size()>4000)
      w.position_history.pop_front();
}

template<typename T>
static T smin(T a,T b,T k)
{
   return -std::log(std::exp(-k*a)+std::exp(-k*b))/k;
}//from iq

double FrostScene::sceneDistance(const Vec3& p)
{
   static const double k = 10.0;
   double dist = 1e3;

   for(int i=0;i<num_boxes;++i)
   {
      const Box& b=boxes[i];
      const Vec3 pt = b.xfrm_inv * p;

      double dx=std::max(0.0f, std::abs(pt.x) - b.size.x);
      double dy=std::max(0.0f, std::abs(pt.y) - b.size.y);
      double dz=std::max(0.0f, std::abs(pt.z) - b.size.z);

      dist=smin(dist,std::sqrt(dx*dx+dy*dy+dz*dz) - 0.1,k);
   }

   return dist;

   //return std::sqrt(p.lengthSquared()) - 1.0;
}

Vec3 FrostScene::sceneGradient(const Vec3& p)
{
   static const double eps=1e-3;
   double c = sceneDistance(p);
   return Vec3(sceneDistance(p + Vec3(eps, 0.0, 0.0)) - c,
               sceneDistance(p + Vec3(0.0, eps, 0.0)) - c,
               sceneDistance(p + Vec3(0.0, 0.0, eps)) - c) * (1.0 / eps);
}

void FrostScene::stepWorm(Worm& w, int i)
{
   addHistory(w);

   Vec3 target=w.position+w.direction*1.0;

   const double d = sceneDistance(target);
   Vec3 n = sceneGradient(target);

   normalize(n);

   target -= n * d;

   w.direction = target - w.position;
   normalize(w.direction);

   w.position = target;

   Vec3 tang=n.cross(w.direction);

   normalize(tang);

   w.direction+=tang*0.2*randomGridLookup(i,w.step_count*0.02);
   normalize(w.direction);

   ++w.step_count;
}

void FrostScene::initWorms()
{
   for(int i=0;i<num_worms;++i)
   {
      worms[i].position=Vec3(frand()-0.5,frand()-0.5,frand()-0.5);
      worms[i].direction=Vec3(frand()-0.5,frand()-0.5,frand()-0.5);
   }
}

void FrostScene::initBoxes()
{
   Ran lrnd(639);
   for(int i=0;i<num_boxes;++i)
   {
/*
      boxes[i].size = Vec3(1.0,0.2,1.0);
      boxes[i].xfrm = Mat4::translation(Vec3((frand()-0.5)*7.0,(frand()-0.5),(frand()-0.5)*7.0));

*/
      boxes[i].size = Vec3(0.5,0.2,0.5);
      boxes[i].xfrm = Mat4::rotation(lrnd.doub()*M_PI*2,Vec3(0,1,0)) *  Mat4::rotation(lrnd.doub()*M_PI*2,Vec3(1,0,0)) *  Mat4::translation(Vec3((frand()-0.5)*3.0,(frand()-0.5),(frand()-0.5)*3.0));

      boxes[i].xfrm_inv = boxes[i].xfrm.inverse();
   }
}


void FrostScene::initRandomGrid()
{
   for(int y=0;y<random_grid_h;++y)
      for(int x=0;x<random_grid_w;++x)
      {
         random_grid[x+y*random_grid_w]=(frand()-0.5)*2.0;
      }
}

float FrostScene::randomGridLookup(float u, float v) const
{
   u=fmodf(u,float(random_grid_w));
   v=fmodf(v,float(random_grid_h));

   if(u<0)
      u+=random_grid_w;

   if(v<0)
      v+=random_grid_h;

   const int iu=int(u);
   const int iv=int(v);

   const float fu=u-float(iu);
   const float fv=v-float(iv);

   const float a=random_grid[iu+iv*random_grid_w];
   const float b=random_grid[((iu+1)%random_grid_w)+iv*random_grid_w];
   const float c=random_grid[((iu+1)%random_grid_w)+((iv+1)%random_grid_h)*random_grid_w];
   const float d=random_grid[iu+((iv+1)%random_grid_h)*random_grid_w];

   return (1.0-fv)*((1.0-fu)*a+fu*b) + fv*((1.0-fu)*d+fu*c);
}

void FrostScene::slowInitialize()
{
   initRandomGrid();
   initWorms();
   initBoxes();

   for(int i=0;i<num_boxes;++i)
   {
      Mesh cube;
      cube.generateCube();
      cube.transform(Mat4::scale(boxes[i].size));
      scene_mesh.addInstance(cube, boxes[i].xfrm);
   }

   icosahedron_mesh.generateIcosahedron();

   initializeShaders();
}


void FrostScene::initializeTextures()
{
   glGenTextures(num_texs, texs);

   createWindowTexture(texs[0], GL_RGBA16F);
   createWindowTexture(texs[2], GL_RGBA16F);
   createWindowTexture(texs[4], GL_RGBA16F, 4);
   createWindowTexture(texs[5], GL_RGBA16F, 4);
   createWindowTexture(texs[6], GL_RGBA16F);
   createWindowTexture(texs[9], GL_RGBA16F, 4);
   createWindowTexture(texs[12], GL_RGBA16F, 2);
   createWindowTexture(texs[13], GL_RGBA16F, 2);

   CHECK_FOR_ERRORS;

   glBindTexture(GL_TEXTURE_2D, 0);

   noise_tex_2d = texs[11];
   bokeh_temp_tex_1 = texs[12];
   bokeh_temp_tex_2 = texs[13];

   {
      uint w = 128, h = 128;
      uint pix[w*h];

      for(uint y = 0; y < h; ++y)
         for(uint x=0;x<w;++x)
         {
            pix[x+y*w]=(lrand()&255) | ((lrand()&255) <<8)|((lrand()&255))<<16;
         }

      glBindTexture(GL_TEXTURE_2D, noise_tex_2d);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pix);
      CHECK_FOR_ERRORS;
   }

}

void FrostScene::initializeBuffers()
{
   glGenFramebuffers(num_fbos, fbos);
   glGenRenderbuffers(num_fbos, renderbuffers);

   bokeh_temp_fbo = fbos[12];

   glBindRenderbuffer(GL_RENDERBUFFER, renderbuffers[1]);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, window_width, window_height);
   CHECK_FOR_ERRORS;
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[0], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[3]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[2], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[4]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[4], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[5]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[5], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[6]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[6], 0);
   glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffers[1]);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[9]);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texs[9], 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bokeh_temp_fbo);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bokeh_temp_tex_1, 0);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bokeh_temp_tex_2, 0);
   assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

   CHECK_FOR_ERRORS;
}

void FrostScene::initializeShaders()
{
   lightning_shader.load(SHADERS_PATH "lines_lightning_v.glsl", SHADERS_PATH "lines_lightning_g.glsl", SHADERS_PATH "lines_lightning_f.glsl");

   forrest_shader.load(SHADERS_PATH "forrest_v.glsl", NULL, SHADERS_PATH "forrest4_f.glsl");

   forrest2_shader.load(SHADERS_PATH "forrest2_v.glsl", NULL, SHADERS_PATH "forrest2_f.glsl");

   bokeh_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "bokeh_f.glsl");
   bokeh_shader.uniform1i("tex0", 0);
   bokeh_shader.uniform1f("aspect_ratio", float(window_height) / float(window_width));

   bokeh2_shader.load(SHADERS_PATH "generic_v.glsl", NULL,SHADERS_PATH "bokeh2_f.glsl");
   bokeh2_shader.uniform1i("tex1", 0);
   bokeh2_shader.uniform1i("tex2", 1);
   bokeh2_shader.uniform1f("aspect_ratio", float(window_height) / float(window_width));

   gaussbokeh_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "gaussblur_f.glsl");
   gaussbokeh_shader.uniform1i("tex0", 0);

   mb_accum_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "mb_accum_f.glsl");
   mb_accum_shader.uniform1i("tex0", 0);


   composite_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "cubes_composite_f.glsl");
   composite_shader.uniform1i("tex0", 0);
   composite_shader.uniform1i("tex1", 1);
   composite_shader.uniform1i("bloom_tex", 4);
   composite_shader.uniform1i("bloom2_tex", 5);

   screendisplay_shader.load(SHADERS_PATH "lines_frost_v.glsl", SHADERS_PATH "lines_frost_g.glsl", SHADERS_PATH "lines_frost_f.glsl");

   bgfg_shader.load(SHADERS_PATH "generic_v.glsl", NULL, SHADERS_PATH "bgfg_f.glsl");
   bgfg_shader.uniform1i("tex0", 0);
   bgfg_shader.uniform1i("tex1", 1);

}



void FrostScene::initialize()
{
   if(initialized)
      return;

   initialized = true;

   initializeTextures();
   initializeBuffers();

   assert(platonic2_tower);
}


void FrostScene::update()
{
   using namespace lightning;
   ++g_ltime;
   step();
   litree0.shrinkOrGrow(-0.5f + pow(noise(0 + fc * grow_shrink_rate), grow_shrink_exp) * max_depth);
   litree1.shrinkOrGrow(-0.5f + pow(noise(10 + fc * grow_shrink_rate), grow_shrink_exp) * max_depth);
}


void FrostScene::drawTower(tower::Tower& twr, const Mat4& modelview, Real camheight)
{
   const Real camheightrange = drawing_view_for_background ? 40 : 10;

   const Real tscale=cubic(clamp((lltime+11.5f)/5.5f,0.0f,1.0f));

   forrest2_shader.bind();

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(2);

   lltime+=open_time; // HAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACK

   unsigned int piece_index=0;
   for(std::vector<tower::PieceInstance>::iterator p=twr.instances.begin();p!=twr.instances.end();++p,++piece_index)
   {
      std::map<std::string,Real>& ps=p->parameters;

      if((ps["background"]>0.5)!=drawing_view_for_background)
         continue;

      if(camheight < (p->section->height1 - camheightrange) || camheight > (p->section->height0 + camheightrange))
         continue;

      Mesh* mesh=&icosahedron_mesh;


      mesh->bind();

      forrest2_shader.uniform1f("time", lltime*ps["time_scale"]+ps["time_offset"]);

      forrest2_shader.uniform1f("ptn_thickness",ps["ptn_thickness"]);
      forrest2_shader.uniform1f("ptn_brightness",ps["ptn_brightness"]);
      forrest2_shader.uniform1f("ptn_scroll",ps["ptn_scroll"]);
      forrest2_shader.uniform1f("ptn_frequency",ps["ptn_frequency"]);
      forrest2_shader.uniform1f("shimmer",ps["shimmer"]);
      forrest2_shader.uniform1f("l0_scroll",ps["l0_scroll"]);
      forrest2_shader.uniform1f("l0_brightness",ps["l0_brightness"]);
      forrest2_shader.uniform1f("l1_scroll",ps["l1_scroll"]);
      forrest2_shader.uniform1f("l1_brightness",ps["l1_brightness"]);
      forrest2_shader.uniform1f("l2_scroll",ps["l2_scroll"]);
      forrest2_shader.uniform1f("l2_brightness",ps["l2_brightness"]);
      forrest2_shader.uniform1f("ambient",ps["ambient"]);

      const float upwards=(piece_index == 27) ? 0.0f : std::pow(std::max(0.0f,(lltime-15.0f-piece_index*0.1f)),2.0f)*0.9f;
      const float rs=(piece_index == 27) ? 0.96f : 1.0f;

      Mat4 modelview2 = modelview * Mat4::rotation(lltime * ps["section_rot_spd"], Vec3(0,1,0)) *
                        Mat4::translation(Vec3(Vec3(std::cos(ps["angle"])*ps["distance"], ps["height"]-upwards, std::sin(ps["angle"])*ps["distance"]) * tscale)) *
                        Mat4::rotation(ps["rot_z"]+lltime*ps["rot_spd_z"]*rs,Vec3(0,0,1)) * Mat4::rotation(ps["rot_y"]+lltime*ps["rot_spd_y"]*rs,Vec3(0,1,0)) * Mat4::rotation(ps["rot_x"]+lltime*ps["rot_spd_x"]*rs,Vec3(1,0,0)) *
                        Mat4::scale(Vec3(ps["sca_x"],ps["sca_y"],ps["sca_z"]));

      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(mesh->getVertexCount() * sizeof(GLfloat) * 3));

      forrest2_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview2.e);

//// 011011
//if(piece_index==27)
      glDrawRangeElements(GL_TRIANGLES, 0, mesh->getVertexCount() - 1, mesh->getTriangleCount() * 3, GL_UNSIGNED_INT, 0);

      CHECK_FOR_ERRORS;
   }

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(2);
}


void FrostScene::createSpiral()
{
   const double s = drawing_view_for_background ? 1.1 : 1;

   if(drawing_view_for_background)
      screendisplay_shader.uniform4f("colour", 0.5 * 0.6 * 0.5, 0.7 * 0.8 * 0.5, 1 * 0.6 * 0.5, 1);
   else
      screendisplay_shader.uniform4f("colour", 0.5, 0.7, 1, 1);

   for(int i=0;i<num_worms;++i)
   {
      spiral_num_vertices = 0;
      spiral_num_indices = 0;

      for(std::list<Vec3>::const_iterator it=worms[i].position_history.begin();it!=worms[i].position_history.end();++it)
      {
         spiral_indices[spiral_num_indices++] = spiral_num_vertices;

         spiral_vertices[spiral_num_vertices * 3 + 0] = it->x * s;
         spiral_vertices[spiral_num_vertices * 3 + 1] = it->y * s;
         spiral_vertices[spiral_num_vertices * 3 + 2] = it->z * s;

         ++spiral_num_vertices;
      }

      glDrawRangeElements(GL_LINE_STRIP, 0, spiral_num_vertices - 1, spiral_num_indices, GL_UNSIGNED_SHORT, spiral_indices);
   }
}

void FrostScene::drawLightning(float ltime)
{
   using namespace lightning;

   fc = ltime*1000.0/20.0;

   GLushort num_vertices=0;

   const float flup=std::pow(std::max(0.0f,ltime-open_time)*0.4f,2.0f);

   Mat4 projection = Mat4::identity();
   Mat4 modelview = Mat4::translation(Vec3(0,-flup*0.5f,0)) * Mat4::scale(Vec3(float(window_height)/float(window_width),1.0f,1.0f));

   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);
   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE,GL_ONE);

   lightning_shader.bind();
   lightning_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
   lightning_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   lightning_shader.uniform1f("time",ltime);
   lightning_shader.uniform2f("radii", 0.01, 0.01);
   lightning_shader.uniform4f("colour", 1,1,1,1);
   lightning_shader.uniform1f("aspect_ratio",float(window_height)/float(window_width));

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);

   glBindBuffer(GL_ARRAY_BUFFER,0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, lightning_vertices);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, lightning_colours);

   litree0.render(num_vertices, lightning_vertices, lightning_colours);
   lightning_shader.uniformMatrix4fv("projection", 1, GL_FALSE, (projection*Mat4::scale(Vec3(-1,1,1))).e);
   litree1.render(num_vertices, lightning_vertices, lightning_colours);

   assert(num_vertices<=lightning_max_vertices);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
}

void FrostScene::drawView(float ltime)
{
   lltime = ltime;

   const float aspect_ratio = float(window_height) / float(window_width);

   const float jitter_x = frand() / window_width * 3;
   const float jitter_y = frand() / window_height * aspect_ratio * 3;

   const float fov = (drawing_view_for_background ? M_PI * 0.145f : M_PI * 0.15f) * mix(2.0f,1.0f,ltime/(ltime+1.0f));

   const float znear = 1.0f / tan(fov * 0.5f), zfar = 1024.0f;

   Mat4 projection = Mat4::frustum(-1.0f + jitter_x, +1.0f + jitter_x, -aspect_ratio + jitter_y, +aspect_ratio + jitter_y, znear, zfar);

   const float move_back = drawing_view_for_background ? 0 : 0;

   const float flup=std::pow(std::max(0.0f,ltime-open_time)*0.5f,2.0f);

   Mat4 modelview = Mat4::rotation(cos(ltime * 15.0) * 0.01 / 128, Vec3(0.0f, 1.0f, 0.0f));
   modelview = modelview * Mat4::rotation(sin(ltime * 17.0) * 0.01 / 128, Vec3(1.0f, 0.0f, 0.0f)) * Mat4::translation(Vec3(0, -10 - flup, -15 - move_back + ltime * 0.02));

   modelview = Mat4::rotation(0.6, Vec3(1.0f,0.0f, 0.0f)) * modelview * Mat4::rotation(-0.5f+ltime*0.04, Vec3(0.0f, 1.0f, 0.0f));

   glViewport(0, 0, window_width, window_height);

   CHECK_FOR_ERRORS;

   glDepthMask(GL_TRUE);
   glEnable(GL_CULL_FACE);
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);

   glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

   CHECK_FOR_ERRORS;

   forrest_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);
   screendisplay_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);


   CHECK_FOR_ERRORS;


   glUseProgram(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);

   if(!drawing_view_for_background)
   {
      drawLightning(ltime);
   }

   glEnable(GL_CULL_FACE);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);

   //if(!drawing_view_for_background)
   {
      static const float pivot_distance=-3.0f;
      static const float pivot_speed=0.6f;
      const float pivot_amount=std::pow(clamp((ltime-open_time)*0.3f,0.0f,1.0f),0.5f);

      forrest_shader.bind();

      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(2);

      Mesh* mesh=&scene_mesh;

      mesh->bind();

      glDisable(GL_BLEND);
      glDisable(GL_CULL_FACE);

      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(mesh->getVertexCount() * sizeof(GLfloat) * 3));

      //forrest_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, modelview.e);
      forrest_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (modelview * Mat4::translation(Vec3(0,0,+pivot_distance)) * Mat4::rotation(+pivot_amount*pivot_speed,Vec3(0,1,0)) * Mat4::translation(Vec3(0,0,-pivot_distance))).e);
      forrest_shader.uniform1f("clip_side", +1.0f);

      glDrawRangeElements(GL_TRIANGLES, 0, mesh->getVertexCount() - 1, mesh->getTriangleCount() * 3, GL_UNSIGNED_INT, 0);

      forrest_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (modelview * Mat4::translation(Vec3(0,0,+pivot_distance)) * Mat4::rotation(-pivot_amount*pivot_speed,Vec3(0,1,0)) * Mat4::translation(Vec3(0,0,-pivot_distance))).e);
      forrest_shader.uniform1f("clip_side", -1.0f);

      glDrawRangeElements(GL_TRIANGLES, 0, mesh->getVertexCount() - 1, mesh->getTriangleCount() * 3, GL_UNSIGNED_INT, 0);

      glEnable(GL_BLEND);
      glEnable(GL_CULL_FACE);

      glDepthMask(GL_FALSE);

      CHECK_FOR_ERRORS;

      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(2);

      //glEnable( GL_CLIP_PLANE0 );
      glEnable(GL_CLIP_DISTANCE0);

      screendisplay_shader.bind();
      screendisplay_shader.uniform1f("brightness", 1);
      screendisplay_shader.uniform1f("time", ltime);
      screendisplay_shader.uniform2f("radii", 0.005, 0.005);
      glDepthMask(GL_FALSE);

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);

      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, spiral_vertices);
      screendisplay_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (modelview * Mat4::translation(Vec3(0,0,+pivot_distance)) * Mat4::rotation(+pivot_amount*pivot_speed,Vec3(0,1,0)) * Mat4::translation(Vec3(0,0,-pivot_distance))).e);
      screendisplay_shader.uniform1f("clip_side", +1.0f);
      createSpiral();
      screendisplay_shader.uniformMatrix4fv("modelview", 1, GL_FALSE, (modelview * Mat4::translation(Vec3(0,0,+pivot_distance)) * Mat4::rotation(-pivot_amount*pivot_speed,Vec3(0,1,0)) * Mat4::translation(Vec3(0,0,-pivot_distance))).e);
      screendisplay_shader.uniform1f("clip_side", -1.0f);
      createSpiral();
      glDisableVertexAttribArray(0);

      glDisable(GL_CLIP_DISTANCE0);
      //glDisable(GL_CLIP_PLANE0);
      glDisable(GL_BLEND);

   }

   if(!drawing_view_for_background && ltime > open_time)
   {
      drawHangingBalls(ltime - open_time);
   }


   glDepthMask(GL_TRUE);

   CHECK_FOR_ERRORS;
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void FrostScene::drawHangingBalls(float ltime)
{
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);

   ltime-=11.5f;

   lltime = ltime;

   const float aspect_ratio = float(window_height) / float(window_width);

   const float jitter_x = frand() / window_width * 3;
   const float jitter_y = frand() / window_height * aspect_ratio * 3;

   const float fov = M_PI * 0.3f;

   const float znear = 1.0f / tan(fov * 0.5f), zfar = 256.0f;

   const float frustum_scale = 1.0f/512.0f;

   Mat4 projection = Mat4::frustum((-1.0f + jitter_x)*frustum_scale, (+1.0f + jitter_x)*frustum_scale, (-aspect_ratio + jitter_y)*frustum_scale, (+aspect_ratio + jitter_y)*frustum_scale, znear*frustum_scale, zfar);

   const float move_back = drawing_view_for_background ? 10 : 0;

   forrest2_shader.uniformMatrix4fv("projection", 1, GL_FALSE, projection.e);

   ltime+=open_time; // HAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACK
   Mat4 modelview = Mat4::rotation(cos(ltime * 15.0) * 0.01 / 128, Vec3(0.0f, 1.0f, 0.0f)) * Mat4::rotation(sin(ltime * 17.0) * 0.01 / 128, Vec3(1.0f, 0.0f, 0.0f)) *
                  Mat4::translation(Vec3(0, 0, -45 - move_back + ltime * 0.2));

   drawTower(*platonic2_tower, modelview, 0);
}


void FrostScene::gaussianBlur(Real tint_r, Real tint_g, Real tint_b, Real radx, Real rady, GLuint fbo0, GLuint fbo1, GLuint src_tex, GLuint tex0)
{
   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);

   glViewport(0, 0, window_width / 4, window_height / 4);

   // gaussian blur pass 1

   {
      gaussbokeh_shader.uniform4f("tint", tint_r, tint_g, tint_b, 1.0f);
   }

   const float gauss_radx = radx;
   const float gauss_rady = rady;

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo0);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }


   CHECK_FOR_ERRORS;

   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);

   gaussbokeh_shader.bind();
   gaussbokeh_shader.uniform2f("direction", gauss_radx * float(window_height) / float(window_width), 0.0f);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, src_tex);

   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);



   // gaussian blur pass 2

   gaussbokeh_shader.uniform4f("tint", 1, 1, 1, 1);


   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo1);

   {
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
   }


   CHECK_FOR_ERRORS;

   gaussbokeh_shader.uniform2f("direction", 0.0f, gauss_rady);

   glBindTexture(GL_TEXTURE_2D, tex0);

   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   CHECK_FOR_ERRORS;
}


void FrostScene::render()
{
   static const GLfloat vertices[] = { -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f };
   static const GLfloat coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
   static const GLubyte indices[] = { 3, 0, 2, 1 };

   const float aspect_ratio = float(window_height) / float(window_width);

   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

   for(int nn = 0; nn < 2; ++nn)
   {
//if(drawing_view_for_background) continue; // **********************************************************************************

      drawing_view_for_background = !nn;

      // ----- motion blur ------

      const float cc=(1.0f-cubic(clamp((time-9.5f)/2.0f,0.0f,1.0f)))*1.5f;

      glClearColor(0.0006f*cc, 0.0012f*cc, 0.003f*cc, 0.0f);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[3]);
      glClear(GL_COLOR_BUFFER_BIT);

      num_subframes = drawing_view_for_background ? 1 : 7;
      subframe_scale = 1.0/35.0;

      for(int subframe = 0;subframe<num_subframes;++subframe)
      {
         // draw the view

         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[6]);

         {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
         }

         glDisable(GL_BLEND);

         drawView(time + subframe_scale * float(subframe) / float(num_subframes));


         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[3]);

         mb_accum_shader.bind();
         mb_accum_shader.uniform1f("time", float(subframe) / float(num_subframes));
         mb_accum_shader.uniform1f("scale", 1.0f / float(num_subframes));

         glDepthMask(GL_FALSE);
         glDisable(GL_DEPTH_TEST);
         glEnable(GL_BLEND);
         glBlendFunc(GL_ONE, GL_ONE);

         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, texs[6]);
         glEnableVertexAttribArray(0);
         glEnableVertexAttribArray(1);
         glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
         glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
         glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
         glDisableVertexAttribArray(0);
         glDisableVertexAttribArray(1);
      }

      CHECK_FOR_ERRORS;

      if(drawing_view_for_background)
      {
         // ----- aperture ------

         const float CoC = 0.02;// + sin(time) * 0.05;

         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bokeh_temp_fbo);

         {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
         }

         glViewport(0, 0, window_width / 2, window_height / 2);

         bokeh_shader.bind();
         bokeh_shader.uniform2f("CoC", CoC / aspect_ratio, CoC);

         glDepthMask(GL_FALSE);
         glDisable(GL_DEPTH_TEST);
         glDisable(GL_BLEND);

         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, texs[2]);
         glEnableVertexAttribArray(0);
         glEnableVertexAttribArray(1);
         glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
         glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
         glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
         glDisableVertexAttribArray(0);
         glDisableVertexAttribArray(1);

         CHECK_FOR_ERRORS;


         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);

         glViewport(0, 0, window_width, window_height);

         {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(sizeof(buffers) / sizeof(buffers[0]), buffers);
         }

         bokeh2_shader.bind();
         bokeh2_shader.uniform2f("CoC", CoC / aspect_ratio, CoC);
         bokeh2_shader.uniform1f("brightness", 0.2);

         glActiveTexture(GL_TEXTURE1);
         glBindTexture(GL_TEXTURE_2D, bokeh_temp_tex_2);
         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, bokeh_temp_tex_1);
         glEnableVertexAttribArray(0);
         glEnableVertexAttribArray(1);
         glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
         glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
         glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
         glDisableVertexAttribArray(0);
         glDisableVertexAttribArray(1);
      }
      else
      {
         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
         glViewport(0, 0, window_width, window_height);
         glDisable(GL_BLEND);
         //glEnable(GL_BLEND);
         //glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);

         bgfg_shader.bind();

         glActiveTexture(GL_TEXTURE1);
         glBindTexture(GL_TEXTURE_2D, texs[2]);
         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, texs[0]);
         glEnableVertexAttribArray(0);
         glEnableVertexAttribArray(1);
         glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
         glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
         glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
         glDisableVertexAttribArray(0);
         glDisableVertexAttribArray(1);

         //glBindFramebuffer(GL_READ_FRAMEBUFFER, fbos[0]);
         //glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[0]);
         //glViewport(0, 0, window_width, window_height);
         //glBlitFramebuffer(0, 0, window_width, window_height, 0, 0, window_width, window_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
      }

      CHECK_FOR_ERRORS;
   }

   // ----- bloom ------

   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_BLEND);

   downsample8x(texs[0], fbos[3], texs[2], fbos[4], window_width, window_height);

   gaussianBlur(1, 1, 1, 0.5, 0.5, fbos[5], fbos[9], texs[4], texs[5]);
   gaussianBlur(1, 1, 1, 0.1, 0.1, fbos[5], fbos[4], texs[4], texs[5]);

   CHECK_FOR_ERRORS;


   // ----- composite ------


   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   glViewport(0, 0, window_width, window_height);


   composite_shader.bind();
   composite_shader.uniform1f("time", time);
   composite_shader.uniform1i("frame_num", frame_num);
   composite_shader.uniform4f("tint", 1, 1, 1, 1);
   {
      float s=std::pow(1.0f-clamp(time,0.0f,1.0f),2.0f);
      composite_shader.uniform3f("addcolour", s,s,s);
   }

   glActiveTexture(GL_TEXTURE5);
   glBindTexture(GL_TEXTURE_2D, texs[9]);
   glActiveTexture(GL_TEXTURE4);
   glBindTexture(GL_TEXTURE_2D, texs[4]);
   glActiveTexture(GL_TEXTURE3);
   glBindTexture(GL_TEXTURE_2D, texs[13]);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, noise_tex_2d);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texs[0]);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, coords);
   glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_BYTE, indices);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   CHECK_FOR_ERRORS;
}


void FrostScene::free()
{
   glDeleteTextures(num_texs, texs);
   glDeleteFramebuffers(num_fbos, fbos);
}

